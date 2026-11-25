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
has ended, as one block. While the run lasts the reporter captures
``sys.stdout`` and ``sys.stderr``, from its first feature on, so that what
the suite prints stays with the scenario it belongs to, in its order among
the step lines. A scenario that passed is one collapsed group whose title has
its mark and time; one that did not pass is not grouped, so that it shows,
and gets an ``::error`` annotation at its failing step. Output between
scenarios, as a test case's own at its start or a feature hook's, is a
``setup output`` group before the next scenario; what follows the last one
is printed as it is before the summary, and what precedes the first feature
is not captured. Step times line up within a scenario. Output that does not
go through ``sys.stdout`` or ``sys.stderr``, as a child process writing to
the inherited file descriptors, is not captured and shows where it happens.
When a reporter method raises, or the run is interrupted, the capture ends
and what it held is printed.

Workflow-command neutralisation: GitHub runs a line as a workflow command if
it starts with ``::``, or with ``##[`` in the older form, after whitespace.
In ``github`` mode every line the reporter prints, the captured output
included, that starts so after leading blanks, control and format
characters (as a byte order mark or a CR) and colour codes gets a space
inside that start, ``: :`` or ``# #[``, so that it shows as text and does not
run. The reporter's own ``::group::``, ``::endgroup::`` and ``::error`` are
the only commands it prints; their data is escaped. ``step_summary`` is a
file the run's summary is appended to as markdown, as ``$GITHUB_STEP_SUMMARY``.
"""

import contextlib
import functools
import html
import io
import os
import re
import sys
import threading
import time
import unicodedata

from harness import bdd, report, results

PRETTY = "pretty"
GITHUB = "github"
MODES = (PRETTY, GITHUB)

PASSED, FAILED, ERROR = bdd.PASSED, bdd.FAILED, bdd.ERROR
# The step an interrupted scenario ends with.
INTERRUPTED = "interrupted"
# A step's text is padded to this width before its time, in pretty mode.
STEP_WIDTH = 72
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


class _Capture(io.TextIOBase):
    """Stands in for sys.stdout or sys.stderr: what is written to either goes
    to one list, in order. ``fileno`` is the original's, so a child process
    given it writes there, past the capture."""

    def __init__(self, entries, lock, original):
        super().__init__()
        self._entries = entries
        self._lock = lock
        self._original = original

    @property
    def encoding(self):
        return getattr(self._original, "encoding", None) or "utf-8"

    def writable(self):
        return True

    def write(self, text):
        if not isinstance(text, str):
            raise TypeError(f"write() argument must be str, not {type(text).__name__}")
        with self._lock:  # the reader threads of applications write too
            self._entries.append(text)
        return len(text)

    def isatty(self):
        return False

    def fileno(self):
        return self._original.fileno()


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
        self._lock = threading.Lock()
        self._originals = None  # (stdout, stderr) while capturing
        self._interrupting = False

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
            self._print_setup_output()
        else:
            self._write(f"Scenario: {title}")

    @_releasing
    def step(self, result, text, message, duration, location=None):
        self.steps[result] = self.steps.get(result, 0) + 1
        step = report.Step(result, text, message, duration, location)
        if self._current:
            self._current.steps.append(step)
        if self._originals:
            with self._lock:
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

    # -- capture -----------------------------------------------------------------

    def _capture(self):
        """Capture sys.stdout and sys.stderr from now on, in github mode."""
        if self.github and not self._originals:
            self._originals = sys.stdout, sys.stderr
            sys.stdout = _Capture(self._entries, self._lock, sys.stdout)
            sys.stderr = _Capture(self._entries, self._lock, sys.stderr)

    @contextlib.contextmanager
    def _taking(self):
        """What was captured so far, dropped once printed: if printing
        fails, ``_release`` prints it."""
        with self._lock:
            entries = list(self._entries)
        yield entries
        with self._lock:
            del self._entries[: len(entries)]

    def _take(self):
        """What was captured so far, emptied."""
        with self._lock:
            entries = list(self._entries)
            self._entries.clear()
        return entries

    def _release(self):
        """Put the streams back and print what is still captured, as it is."""
        if self._originals:
            sys.stdout, sys.stderr = self._originals
            self._originals = None
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

    def _print_setup_output(self):
        """What was printed since the last scenario, as a collapsed group."""
        with self._taking() as entries:
            lines = self._body(entries, "")
            if any(line.strip() for line in lines):
                count = f"{len(lines)} line{'' if len(lines) == 1 else 's'}"
                self._command(f"::group::setup output ({count})")
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
        self.stream.write(text)
        self.stream.flush()


_ANSI = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")
_COMMAND_STARTS = ("::", "##[")


def neutralise(line):
    """A line that does not run as a workflow command; see the module.

    Leading whitespace, control and format characters (CR, a byte order
    mark, zero-width spaces) and colour codes are skipped; if ``::`` or
    ``##[`` follows, a space goes after its first character.
    """
    start = 0
    while start < len(line):
        if match := _ANSI.match(line, start):
            start = match.end()
        elif line[start].isspace() or unicodedata.category(line[start])[0] in "CZ":
            start += 1
        else:
            break
    for command in _COMMAND_STARTS:
        if line.startswith(command, start):
            return f"{line[:start]}{command[0]} {line[start + 1:]}"
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
