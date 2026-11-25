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

``github`` mode prints the same for GitHub Actions: each scenario is a
collapsed group, its result line and failures follow the group, and every
scenario that did not pass gets an ``::error`` annotation at its failing
step. While the run lasts, workflow commands are stopped except for the
reporter's own, so that nothing the suite or the application prints is run
as one. ``step_summary`` is a file the run's summary is appended to as
markdown, as ``$GITHUB_STEP_SUMMARY``.
"""

import html
import os
import re
import secrets
import sys
import time

from harness import bdd, report, results

PRETTY = "pretty"
GITHUB = "github"
MODES = (PRETTY, GITHUB)

PASSED, FAILED, ERROR = bdd.PASSED, bdd.FAILED, bdd.ERROR
# The step an interrupted scenario ends with.
INTERRUPTED = "interrupted"
# A step's text is padded to this width before its time.
STEP_WIDTH = 72
# The longest message the job summary quotes.
SUMMARY_MESSAGE = 2000

_MARKS = {PASSED: "✓", FAILED: "✗", ERROR: "✗", bdd.SKIPPED: "-"}
_ASCII_MARKS = {PASSED: "+", FAILED: "x", ERROR: "x", bdd.SKIPPED: "-"}
_COLORS = {PASSED: "32", FAILED: "31", ERROR: "33", bdd.SKIPPED: "90", "bold": "1"}


class ConsoleReporter(bdd.Reporter):  # pylint: disable=too-many-instance-attributes
    """Prints the run to ``stream`` as it goes; see the module.

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
        self._token = None  # while workflow commands are stopped
        self._started = time.monotonic()

    # -- bdd.Reporter ------------------------------------------------------------

    def feature_start(self, title, path):
        self._stop_commands()
        self._close()
        self.features.append(report.Feature(title, path))
        heading = f"Feature: {title}  ({self._path(path)})"
        self._write("", self._paint("bold", heading))

    def scenario_start(self, title, tags, location=None):
        self._stop_commands()
        self._close()
        if not self.features:
            self.features.append(report.Feature("", ""))
        self._current = self._last = report.Scenario.start(title, tags, location)
        self.features[-1].scenarios.append(self._current)
        if self.github:
            self._command(f"::group::{escape_data(title)}")
        else:
            self._write(f"Scenario: {title}")

    def step(self, result, text, message, duration, location=None):
        self.steps[result] = self.steps.get(result, 0) + 1
        if self._current:
            self._current.steps.append(
                report.Step(result, text, message, duration, location)
            )
        timing = "skipped" if result == bdd.SKIPPED else f"{duration:.1f} s"
        line = f"  {self.marks.get(result, '?')} {text}".ljust(STEP_WIDTH)
        self._write(self._paint(result, f"{line}  {timing}"))
        if message and result in (FAILED, ERROR):
            self._write(_indent(message, 6))

    def attach(self, path):
        if scenario := self._current or self._last:
            scenario.attach(path)

    def scenario_end(self, record):
        if not (scenario := self._current):
            return
        self._current = None
        scenario.end(record)
        if self.github:
            self._command("::endgroup::")
        mark = self.marks[scenario.status]
        timing = f"{scenario.duration:.1f} s"
        if scenario.status != PASSED:
            timing = f"{scenario.status}, {timing}"
        self._write(
            self._paint(scenario.status, f"{mark} {scenario.title}  ({timing})")
        )
        if scenario.status == PASSED:
            return
        problems = scenario.problems()
        for problem in problems:
            where = f"  ({self._path(*problem.location)})" if problem.location else ""
            self._write(f"    {problem.text}{where}")
            if problem.message:
                self._write(_indent(problem.message, 4))
        for path in scenario.attachments:
            self._write(f"    attachment: {path}")
        if self.github:
            self._annotate(scenario, problems)

    def interrupted(self, message):
        """End the running scenario as an error: the run stopped in it."""
        if self._current:
            self.step(ERROR, INTERRUPTED, message, 0.0)
            self.scenario_end(results.current())

    def finish(self):
        """Print the summary, append the job summary, let commands run again."""
        self._close()
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
        if self._token:
            self._write(f"::{self._token}::")
            self._token = None

    # -- output ------------------------------------------------------------------

    def _failing(self):
        """(feature, scenario) for each scenario that did not pass."""
        return [
            (f, s) for f in self.features for s in f.scenarios if s.status != PASSED
        ]

    def _close(self):
        """End a scenario that is still open, so that its group closes.

        It did not finish: the results the runner records by now are not its.
        """
        if self._current:
            self.scenario_end(None)

    def _stop_commands(self):
        """Stop workflow commands for the run, once, in github mode."""
        if self.github and self._token is None:
            self._token = secrets.token_hex(16)
            self._write(f"::stop-commands::{self._token}")

    def _command(self, *commands):
        """Run workflow commands of the reporter's own, between stops."""
        if self._token:
            self._write(
                f"::{self._token}::", *commands, f"::stop-commands::{self._token}"
            )
        else:
            self._write(*commands)

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
        text = "\n".join(lines) + "\n"
        if self.narrow:
            text = text.encode(self.narrow, "replace").decode(self.narrow)
        self.stream.write(text)
        self.stream.flush()


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
