# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""The run on the console, as Gherkin runners print it: features, scenarios
and their steps, as they finish.

``ConsoleReporter`` is a ``bdd.Reporter``. In ``pretty`` mode each scenario
starts with a ``Scenario:`` heading, so that what the suite prints while a
step runs appears between that heading and the step's line. Each step's line
follows when the step has finished, with its mark and time; the scenario's
result line follows its steps, and for a scenario that did not pass the
failing steps with their locations and messages. A summary of scenarios and
steps ends the run.

``github`` mode prints for GitHub Actions, and prints each scenario when it
has ended, as one block. From its construction until ``finish`` or
``close`` the reporter captures file descriptors 1 and 2, as one pipe, so
that everything the process and its children write to either, through
``sys.stdout``, ``sys.stderr``, a log handler or an inherited descriptor,
stays with the scenario it belongs to, in its order among the step lines;
before printing anything, the reporter waits until what was written so far
has arrived. Its own lines go to ``stream``, past the capture. A scenario
that passed is one collapsed group whose title has its mark and time; one
that did not pass is not grouped, so that it shows, and gets an ``::error``
annotation at its failing step. Output between scenarios, as a test case's
own at its start or a feature hook's, is a ``setup output`` group before the
next scenario; what follows the last one is printed as it is before the
summary, or as a ``run output`` group if no scenario ran. Step times line up
within a scenario. When a reporter method raises, or the run is
interrupted, the descriptors are put back and what was captured is printed.
Nothing shows while a scenario runs; a run killed with SIGKILL loses the
running scenario's output (the reports and application logs keep theirs).
A process that still holds the pipe after the capture ended, as a child left
running, gets SIGPIPE when it writes (EPIPE if it ignores the signal).

Workflow-command neutralisation: GitHub runs a line as a workflow command if
it starts with ``::`` after whitespace, and takes the older ``##[name]``
form anywhere in a line. In ``github`` mode every line the reporter prints,
the captured output included, gets ``: :`` for a ``::`` it starts with after
leading blanks, control and format characters (as a byte order mark or a
CR) and colour codes, and ``# #[`` for every ``##[``, so that it shows as
text and does not run. The reporter's own ``::group::``, ``::endgroup::``
and ``::error`` are the only commands it prints; their data is escaped.
``step_summary`` is a file the run's summary is appended to as markdown, as
``$GITHUB_STEP_SUMMARY``.
"""

import atexit
import contextlib
import functools
import html
import os
import re
import secrets
import signal
import sys
import threading
import time
import unicodedata
import weakref

from harness import bdd, report, results

PRETTY = "pretty"
GITHUB = "github"
MODES = (PRETTY, GITHUB)

PASSED, FAILED, ERROR = bdd.PASSED, bdd.FAILED, bdd.ERROR
# The step an interrupted scenario ends with.
INTERRUPTED = "interrupted"
# A step's text is padded to this width before its time, in pretty mode.
STEP_WIDTH = 72
# How long, in seconds, the github console waits for captured output.
SYNC_TIMEOUT = 5.0
# How long, in seconds, it waits for the rest when the capture ends.
STOP_TIMEOUT = 10.0
# The longest message the job summary quotes.
SUMMARY_MESSAGE = 2000

_MARKS = {PASSED: "✓", FAILED: "✗", ERROR: "✗", bdd.SKIPPED: "-"}
_ASCII_MARKS = {PASSED: "+", FAILED: "x", ERROR: "x", bdd.SKIPPED: "-"}
_COLORS = {PASSED: "32", FAILED: "31", ERROR: "33", bdd.SKIPPED: "90", "bold": "1"}


def _releasing(method):
    """End the capture when a reporter method raises, and print what it held.

    A reporter that fails, or is interrupted, must neither keep the streams
    swapped nor lose the output.
    """

    @functools.wraps(method)
    def call(self, *args, **kwargs):
        try:
            return method(self, *args, **kwargs)
        except BaseException:
            self._release()  # pylint: disable=protected-access
            raise

    return call


class _FdCapture:  # pylint: disable=too-many-instance-attributes
    """Captures file descriptors 1 and 2, as one pipe, until ``stop``.

    What the process and its children write to either arrives in
    ``entries``, a line at a time, in the order it was written. ``sync``
    waits until everything written so far has arrived. ``sys.stdout`` and
    ``sys.stderr`` are made to write to the descriptors: line-buffered, or,
    where they are no streams over them, replaced by ones that are.
    ``output(fd)`` writes to what the descriptor was before.
    """

    def __init__(self, entries, condition):
        self._entries = entries
        self._condition = condition
        self._prefix = f"\0console-sync-{secrets.token_hex(8)}-".encode()
        self._sent = self._seen = 0
        self._streams = []  # (name, original, its line_buffering, replacement)
        self._outputs = []
        _flush(sys.stdout, sys.stderr)
        self.saved = {fd: os.dup(fd) for fd in (1, 2)}
        self._read_fd, self._write_fd = os.pipe()
        try:
            for fd in (1, 2):
                os.dup2(self._write_fd, fd)
            self._point_streams()
        except BaseException:
            with _signals_blocked():
                self._restore()
            self._close_fds(self._read_fd, *self.saved.values())
            raise
        self._thread = threading.Thread(
            target=self._read, name="console capture", daemon=True
        )
        self._thread.start()

    def output(self, fd, encoding):
        """A text stream to what ``fd`` was before the capture."""
        # pylint: disable-next=consider-using-with
        stream = open(
            self.saved[fd],
            "w",
            encoding=encoding or "utf-8",
            errors="replace",
            closefd=False,
        )
        self._outputs.append(stream)
        return stream

    def sync(self):
        """Wait until what was written so far has arrived, at most
        ``SYNC_TIMEOUT``; after that, a line says that output may be missing."""
        _flush(sys.stdout, sys.stderr)
        self._sent += 1
        number = self._sent
        os.write(self._write_fd, self._prefix + str(number).encode() + b"\n")
        with self._condition:
            if not self._condition.wait_for(lambda: self._seen >= number, SYNC_TIMEOUT):
                self._entries.append(
                    f"[WARNING] the console capture is {SYNC_TIMEOUT:.0f} s behind; "
                    "output may be incomplete or out of order\n"
                )

    def stop(self):
        """Sync, put the descriptors and streams back and end the reader.

        Each step happens whatever the one before did; SIGINT and SIGTERM
        wait until the descriptors and streams are back. The first error of
        putting them back is raised at the end.
        """
        errors = []
        try:
            self.sync()
        finally:
            try:
                with _signals_blocked():
                    errors = self._restore()
            finally:
                self._end_reader()
        if errors:
            raise errors[0]

    def _end_reader(self):
        """Let the reader take the rest and end; close what the capture opened."""
        try:
            try:
                os.write(self._write_fd, self._prefix + b"stop\n")
            except OSError:
                pass
            self._close_fds()  # the write end: EOF, once no child holds it
            self._thread.join(STOP_TIMEOUT)
            if self._thread.is_alive():
                with self._condition:
                    self._entries.append(
                        "[WARNING] the console capture did not end within "
                        f"{STOP_TIMEOUT:.0f} s; output may be incomplete\n"
                    )
        finally:
            for stream in self._outputs:
                try:
                    stream.close()
                except (OSError, ValueError):
                    pass
            for fd in self.saved.values():
                _close_quietly(fd)

    def _point_streams(self):
        for name, fd in (("stdout", 1), ("stderr", 2)):
            stream = getattr(sys, name)
            if _fileno(stream) == fd and hasattr(stream, "reconfigure"):
                self._streams.append((name, stream, stream.line_buffering, None))
                stream.reconfigure(line_buffering=True)
            elif _fileno(stream) != fd:
                # pylint: disable-next=consider-using-with
                replacement = open(
                    fd,
                    "w",
                    encoding=getattr(stream, "encoding", None) or "utf-8",
                    errors="backslashreplace",
                    buffering=1,
                    closefd=False,
                )
                _CAPTURE_STREAMS.add(replacement)
                self._streams.append((name, stream, None, replacement))
                setattr(sys, name, replacement)

    def _restore(self):
        """The descriptors, then the streams, as before; returns the errors.

        A sys stream goes back only if it still is the capture's: one
        replaced since stays. The write end stays open.
        """
        errors = []
        _flush(sys.stdout, sys.stderr)
        for fd, saved in self.saved.items():
            try:
                os.dup2(saved, fd)
            except OSError as exc:
                errors.append(exc)
        for name, original, line_buffering, replacement in self._streams:
            try:
                if not replacement:
                    original.reconfigure(line_buffering=line_buffering)
                elif getattr(sys, name) is replacement:
                    # left open: a log handler may hold it; fd 2 is real again
                    setattr(sys, name, original)
            except ValueError:
                pass  # closed by the suite: nothing to put back
            except Exception as exc:  # pylint: disable=broad-exception-caught
                errors.append(exc)
        self._streams.clear()
        return errors

    def _close_fds(self, *fds):
        for fd in (self._write_fd, *fds):
            _close_quietly(fd)

    def _read(self):
        pending, stop = b"", False
        try:
            while not stop:
                if not (chunk := os.read(self._read_fd, 65536)):
                    break
                *lines, pending = (pending + chunk).split(b"\n")
                for line in lines:
                    stop = self._line(line) or stop
        finally:
            with self._condition:
                if pending:
                    self._entries.append(pending.decode("utf-8", "replace") + "\n")
                self._seen = self._sent
                self._condition.notify_all()
            os.close(self._read_fd)

    def _line(self, line):
        """Take a line; returns whether it was the last."""
        at = line.find(self._prefix)
        text = line if at < 0 else line[:at]
        with self._condition:
            if at < 0 or text:
                self._entries.append(text.decode("utf-8", "replace") + "\n")
            if at < 0:
                return False
            if (tag := line[at + len(self._prefix) :]) == b"stop":
                return True
            self._seen = int(tag)
            self._condition.notify_all()
        return False


# the streams over fds 1 and 2 a capture put in place of sys.stdout or sys.stderr
_CAPTURE_STREAMS = weakref.WeakSet()


def is_capture_stream(stream):
    """Whether ``stream`` is one a capture put in place of a sys stream."""
    try:
        return stream in _CAPTURE_STREAMS
    except TypeError:  # cannot be weakly referenced: not one
        return False


@contextlib.contextmanager
def _signals_blocked():
    """SIGINT and SIGTERM held back, where signals can be, until the end."""
    mask = getattr(signal, "pthread_sigmask", None)
    if not mask or threading.current_thread() is not threading.main_thread():
        yield
        return
    previous = mask(signal.SIG_BLOCK, {signal.SIGINT, signal.SIGTERM})
    try:
        yield
    finally:
        mask(signal.SIG_SETMASK, previous)


def _close_quietly(fd):
    try:
        os.close(fd)
    except OSError:
        pass


def _fileno(stream):
    try:
        return stream.fileno()
    except (AttributeError, OSError, ValueError):  # io.UnsupportedOperation too
        return None


def _flush(*streams):
    for stream in streams:
        try:
            stream.flush()
        except (AttributeError, OSError, ValueError):
            pass


class ConsoleReporter(bdd.Reporter):  # pylint: disable=too-many-instance-attributes
    """Prints the run to ``stream``; see the module.

    ``workspace`` is the directory paths are shown relative to, by default
    the current one; GitHub wants annotation files relative to the
    repository. ``color`` None colours in ``github`` mode and where the
    stream is a terminal or ``FORCE_COLOR`` is set, never if ``NO_COLOR``
    is set.
    """

    def __init__(
        self, stream, mode=PRETTY, workspace=None, step_summary=None, color=None
    ):
        if mode not in MODES:
            raise ValueError(f"unknown console mode {mode!r}, expected one of {MODES}")
        self.stream = stream
        self.github = mode == GITHUB
        self.workspace = os.path.realpath(workspace or os.getcwd())
        self.step_summary = step_summary
        self.color = _use_color(stream, self.github) if color is None else color
        encoding = getattr(stream, "encoding", None)
        # where the marks do not encode, neither may the rest
        self.narrow = encoding if encoding and not _encodes(encoding, "✓✗") else None
        self.marks = _ASCII_MARKS if self.narrow else _MARKS
        self.features = []
        self.steps = dict.fromkeys(_MARKS, 0)
        self._current = None
        self._last = None
        self._started = time.monotonic()
        # github mode: what the suite printed and the steps, in order
        self._entries = []
        self._condition = threading.Condition()
        self._fd_capture = None
        self._out = stream  # where the reporter's own lines go
        self._interrupting = False
        self._capture()  # as a test case prints before its first feature

    # -- bdd.Reporter ------------------------------------------------------------

    @_releasing
    def feature_start(self, title, path):
        self._capture()
        self._close()
        self.features.append(report.Feature(title, path))
        heading = f"Feature: {title}  ({self._path(path)})"
        self._write("", self._paint("bold", heading))

    @_releasing
    def scenario_start(self, title, tags, location=None):
        self._capture()
        self._close()
        if not self.features:
            self.features.append(report.Feature("", ""))
        self._current = self._last = report.Scenario.start(title, tags, location)
        self.features[-1].scenarios.append(self._current)
        if self.github:
            self._print_captured("setup output")
        else:
            self._write(f"Scenario: {title}")

    @_releasing
    def step(self, result, text, message, duration, location=None):
        self.steps[result] = self.steps.get(result, 0) + 1
        step = report.Step(result, text, message, duration, location)
        if self._current:
            self._current.steps.append(step)
        if self._fd_capture:
            self._fd_capture.sync()  # what the step printed comes first
            with self._condition:
                self._entries.append(step)
        else:
            self._write(*self._step_lines(step, STEP_WIDTH))

    def attach(self, path):
        if scenario := self._current or self._last:
            scenario.attach(path)

    @_releasing
    def scenario_end(self, record):
        if not (scenario := self._current):
            return
        self._current = None
        scenario.end(record)
        if self.github:
            with self._taking() as entries:
                self._block(scenario, entries)
            return
        mark = self.marks[scenario.status]
        timing = f"{scenario.duration:.1f} s"
        if scenario.status != PASSED:
            timing = f"{scenario.status}, {timing}"
        self._write(
            self._paint(scenario.status, f"{mark} {scenario.title}  ({timing})")
        )
        if scenario.status == PASSED:
            return
        for problem in scenario.problems():
            self._write(f"    {report.titled(problem, self.workspace)}")
            if problem.message:
                self._write(_indent(problem.message, 4))
        for path in scenario.attachments:
            self._write(f"    attachment: {path}")

    @_releasing
    def interrupted(self, message):
        """End the running scenario as an error: the run stopped in it.

        Nothing is captured after: only ``finish`` follows.
        """
        try:
            if self._current:
                self._interrupting = True
                self.step(ERROR, INTERRUPTED, message, 0.0)
                self.scenario_end(results.current())
        finally:
            self._interrupting = False
            self._release()

    @_releasing
    def finish(self):
        """Print the summary and append the job summary."""
        self._close()
        if not any(f.scenarios for f in self.features):
            self._print_captured("run output")
        self._release()
        scenarios = [s for f in self.features for s in f.scenarios]
        totals = report.count_statuses(scenarios)
        elapsed = time.monotonic() - self._started
        self._write(
            "",
            f"{len(scenarios)} scenarios: {totals[PASSED]} passed, "
            f"{totals[FAILED]} failed, {totals[ERROR]} errors ({elapsed:.1f} s)",
            f"{sum(self.steps.values())} steps: {self.steps[PASSED]} passed, "
            f"{self.steps[FAILED]} failed, {self.steps[ERROR]} errors, "
            f"{self.steps[bdd.SKIPPED]} skipped",
        )
        if failing := self._failing():
            self._write("", "Failing scenarios:")
            for feature, scenario in failing:
                where = self._path(*(scenario.location or (feature.path,)))
                self._write(
                    self._paint(scenario.status, f"  {where}  {scenario.title}")
                )
        if self.step_summary:
            self._append_summary(scenarios, totals, elapsed)

    def close(self):
        """End the capture, as for a run that stopped before it began;
        ``finish`` does too."""
        self._release()

    # -- capture -----------------------------------------------------------------

    def _capture(self):
        """Capture file descriptors 1 and 2 from now on, in github mode."""
        if self.github and not self._fd_capture:
            self._fd_capture = _FdCapture(self._entries, self._condition)
            atexit.register(self.close)
            if (fd := _fileno(self.stream)) in (1, 2):
                self.stream.flush()
                self._out = self._fd_capture.output(fd, self.stream.encoding)

    @contextlib.contextmanager
    def _taking(self):
        """What was captured so far, dropped once printed: if printing
        fails, ``_release`` prints it."""
        if self._fd_capture:
            self._fd_capture.sync()
        with self._condition:
            entries = list(self._entries)
        yield entries
        with self._condition:
            del self._entries[: len(entries)]

    def _take(self):
        """What was captured so far, emptied."""
        with self._condition:
            entries = list(self._entries)
            self._entries.clear()
        return entries

    def _release(self):
        """Put the streams back and print what is still captured, as it is."""
        if capture := self._fd_capture:
            try:
                capture.stop()
            finally:
                self._fd_capture = None
                atexit.unregister(self.close)
                self._out = self.stream
        if entries := self._take():
            self._write(*self._body(entries, ""))

    # -- output ------------------------------------------------------------------

    def _failing(self):
        """(feature, scenario) for each scenario that did not pass."""
        return [
            (f, s) for f in self.features for s in f.scenarios if s.status != PASSED
        ]

    def _close(self):
        """End a scenario that is still open, so that it is printed.

        It did not finish: the results the runner records by now are not its.
        """
        if self._current:
            self.scenario_end(None)

    def _print_captured(self, what):
        """What was printed since the last scenario, as a collapsed group."""
        with self._taking() as entries:
            lines = "\n".join(self._body(entries, "")).splitlines()
            if any(line.strip() for line in lines):
                count = f"{len(lines)} line{'' if len(lines) == 1 else 's'}"
                self._command(f"::group::{what} ({count})")
                self._write(*lines)
                self._command("::endgroup::")

    def _block(self, scenario, entries):
        """A scenario that ended, in github mode: a group if it passed, else
        its result, steps, output and failure, and an error annotation."""
        mark = self.marks[scenario.status]
        timing = f"{scenario.duration:.1f} s"
        body = self._body(entries, "  ", located=True)
        if scenario.status == PASSED:
            title = escape_data(f"{mark} {scenario.title}  ({timing})")
            self._command(f"::group::{title}")
            if body:
                self._write(*body)
            self._command("::endgroup::")
            return
        status = INTERRUPTED if self._interrupting else scenario.status
        title = f"{mark} {scenario.title}  ({status}, {timing})"
        self._write(self._paint(scenario.status, title), *body)
        problems = scenario.problems()
        if not any(s.result in (FAILED, ERROR) for s in scenario.steps):
            # failed without a failing step: what failed is not shown yet
            for problem in problems:
                self._write(f"  {report.titled(problem, self.workspace)}")
                if problem.message:
                    self._write(_indent(problem.message, 4))
        for path in scenario.attachments:
            self._write(f"  attachment: {path}")
        self._annotate(scenario, problems)

    def _body(self, entries, indent, located=False):
        """Captured text and step lines, in order; steps' times line up."""
        steps = [e for e in entries if isinstance(e, report.Step)]
        width = max((len(self._step_head(s)) for s in steps), default=0)
        lines, text = [], []
        for entry in entries + [None]:
            if isinstance(entry, str):
                text.append(entry)
                continue
            lines += [indent + line for line in "".join(text).splitlines()]
            text = []
            if entry:
                lines += self._step_lines(entry, width, located)
        return lines

    def _step_head(self, step):
        return f"  {self.marks.get(step.result, '?')} {step.text}"

    def _step_lines(self, step, width, located=False):
        """A step's line, its text padded to ``width`` before its time, and
        its message if it failed; ``located``, with where it is written."""
        timing = "skipped" if step.result == bdd.SKIPPED else f"{step.duration:.1f} s"
        lines = [
            self._paint(step.result, f"{self._step_head(step).ljust(width)}  {timing}")
        ]
        if step.result in (FAILED, ERROR):
            if step.message:
                lines.append(_indent(step.message, 6))
            if located and step.location:
                lines.append(f"      ({self._path(*step.location)})")
        return lines

    def _command(self, *commands):
        """The reporter's own workflow commands, as they are."""
        self._emit("\n".join(commands) + "\n")

    def _annotate(self, scenario, problems):
        """An error annotation at the first problem, in the current feature."""
        problem = problems[0] if problems else None
        location = (problem and problem.location) or scenario.location
        properties = {
            "file": self._path(location.path if location else self.features[-1].path)
        }
        if location:
            properties["line"] = str(location.line)
        properties["title"] = scenario.title
        message = problem.message if problem and problem.message else scenario.status
        self._command(f"::error {_properties(properties)}::{escape_data(message)}")

    def _append_summary(self, scenarios, totals, elapsed):
        lines = [
            f"### GUI tests: {len(scenarios)} scenarios, {totals[PASSED]} passed, "
            f"{totals[FAILED]} failed, {totals[ERROR]} errors ({elapsed:.1f} s)",
            "",
            "| Feature | Passed | Failed | Errors | Time |",
            "| --- | ---: | ---: | ---: | ---: |",
        ]
        for feature in self.features:
            duration = sum(s.duration for s in feature.scenarios)
            lines.append(
                f"| {_md(feature.title)} | {feature.count(PASSED)} | "
                f"{feature.count(FAILED)} | {feature.count(ERROR)} | {duration:.1f} s |"
            )
        if failing := self._failing():
            lines += ["", "#### Failing scenarios"]
        for feature, scenario in failing:
            lines += [
                "",
                f"**{_md(feature.title)}: {_md(scenario.title)}** "
                f"({scenario.status})",
            ]
            for problem in scenario.problems():
                where = (
                    f" <code>{html.escape(self._path(*problem.location))}</code>"
                    if problem.location
                    else ""
                )
                message = problem.message
                if len(message) > SUMMARY_MESSAGE:
                    message = message[:SUMMARY_MESSAGE] + "…"
                lines += [
                    "",
                    f"{_md(problem.text)}{where}",
                    "",
                    f"<pre>{html.escape(message, quote=False)}</pre>",
                ]
        try:
            with open(self.step_summary, "a", encoding="utf-8") as stream:
                stream.write("\n".join(lines) + "\n")
        except OSError as exc:
            print(
                f"[ERROR] cannot write the job summary {self.step_summary}: {exc}",
                file=sys.stderr,
                flush=True,
            )

    def _path(self, path, line=None):
        """A path relative to the workspace if it is inside, with a line."""
        return report.display_path(path, self.workspace, line)

    def _paint(self, kind, text):
        if not self.color or kind not in _COLORS:
            return text
        return f"\x1b[{_COLORS[kind]}m{text}\x1b[0m"

    def _write(self, *lines):
        """Lines of text; in github mode none of them runs as a command."""
        text = "\n".join(lines)
        if self.github:
            text = "\n".join(neutralise(line) for line in text.splitlines())
        self._emit(text + "\n")

    def _emit(self, text):
        if self.narrow:
            text = text.encode(self.narrow, "replace").decode(self.narrow)
        self._out.write(text)
        self._out.flush()


_ANSI = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")


def neutralise(line):
    """A line that does not run as a workflow command; see the module.

    Leading whitespace, control and format characters (CR, a byte order
    mark, zero-width spaces) and colour codes are skipped; if ``::``
    follows, a space goes between its colons. Every ``##[``, which GitHub
    takes anywhere in a line, becomes ``# #[``.
    """
    line = line.replace("##[", "# #[")
    start = 0
    while start < len(line):
        if match := _ANSI.match(line, start):
            start = match.end()
        elif line[start].isspace() or unicodedata.category(line[start])[0] in "CZ":
            start += 1
        else:
            break
    if line.startswith("::", start):
        return f"{line[:start]}: {line[start + 1:]}"
    return line


def escape_data(text):
    """Workflow-command escaping of a command's data: one line, decoded back."""
    return str(text).replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def escape_property(text):
    """Workflow-command escaping of a property's value."""
    return escape_data(text).replace(":", "%3A").replace(",", "%2C")


def _properties(properties):
    return ",".join(
        f"{key}={escape_property(value)}" for key, value in properties.items()
    )


def _indent(text, width):
    return "\n".join(" " * width + line for line in str(text).splitlines())


_MARKDOWN = re.compile(r"([\\`*_\[\]|#~<>])")


def _md(text):
    """Text as itself in markdown, a table cell included."""
    return _MARKDOWN.sub(r"\\\1", " ".join(str(text).splitlines()))


def _encodes(encoding, text):
    try:
        text.encode(encoding)
    except (UnicodeEncodeError, LookupError):
        return False
    return True


def _use_color(stream, github):
    if os.environ.get("NO_COLOR"):
        return False
    if github or os.environ.get("FORCE_COLOR"):
        return True
    isatty = getattr(stream, "isatty", None)
    return bool(isatty and isatty())
