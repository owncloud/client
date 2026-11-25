# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Reports of a run as files: HTML and JUnit XML.

``Report`` is a ``bdd.Reporter``. It collects features, scenarios and their
steps while the suite runs and writes the files when the run finishes, or stops.
Screenshots that hooks save under ``$GUI_TEST_REPORT_DIR/screenshots``
while a scenario runs are attached to that scenario.
"""

import html
import os
import re
import sys
import time
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from typing import NamedTuple
from urllib.parse import quote

from harness import bdd, results

PASSED = "passed"
FAILED = "failed"
ERROR = "error"
# The step an interrupted scenario ends with.
INTERRUPTED = "interrupted"

# Characters XML 1.0 does not allow, e.g. the ESC of colour codes in a log.
_INVALID_XML = re.compile("[^\t\n\r\x20-\ud7ff\ue000-\ufffd\U00010000-\U0010ffff]")


@dataclass
class Step:
    result: str
    text: str
    message: str
    duration: float
    location: object = None  # a bdd.Location, None for a hook


class Problem(NamedTuple):
    """What went wrong in a scenario: a failing step, or a failed result."""

    text: str  # the step's text, or what failed
    location: object  # the step's bdd.Location, or None
    message: str
    detail: str


@dataclass
class Scenario:  # pylint: disable=too-many-instance-attributes
    """One scenario of a run, as reporters collect it.

    ``start`` creates one when the scenario starts, ``end`` completes it;
    one that never ended has no record and is an error.
    """

    title: str
    tags: list
    location: object = None  # a bdd.Location, None for a hook's failures
    steps: list = field(default_factory=list)
    attachments: list = field(default_factory=list)
    record: object = None  # the scenario's results.Results
    duration: float = 0.0
    started: float = 0.0
    screenshots_before: dict = None

    @classmethod
    def start(cls, title, tags, location=None):
        scenario = cls(title, list(tags), location, started=time.monotonic())
        scenario.screenshots_before = _listing(_screenshots_dir())
        return scenario

    def end(self, record):
        """Complete the scenario with its ``results.Results``, None if it did
        not finish, and attach the screenshots saved since it started."""
        self.record = record
        self.duration = time.monotonic() - self.started
        directory = _screenshots_dir()
        for name, mtime in sorted(_listing(directory).items()):
            if (self.screenshots_before or {}).get(name) != mtime:
                self.attachments.append(os.path.join(directory, name))

    def attach(self, path):
        self.attachments.append(os.path.abspath(path))

    @property
    def status(self):
        """A scenario that never ended (it has no record) is an error."""
        if self.record is None:
            return ERROR
        kinds = {e.kind for e in self.record.entries}
        step_results = {s.result for s in self.steps}
        if bdd.ERROR in step_results or "errors" in kinds:
            return ERROR
        if bdd.FAILED in step_results or "fails" in kinds:
            return FAILED
        return PASSED

    def problems(self):
        """What went wrong, as a Problem per failing step.

        A scenario that failed without a failing step, as by ``test.fail``
        in a hook, reports the failed results instead.
        """
        details = {}
        if self.record:
            for entry in self.record.entries:
                if entry.kind in ("fails", "errors") and entry.detail:
                    details.setdefault(entry.message, entry.detail)
        problems = [
            Problem(s.text, s.location, s.message, details.get(s.message, ""))
            for s in self.steps
            if s.result in (bdd.FAILED, bdd.ERROR)
        ]
        if not problems and self.record:
            problems = [
                Problem(e.kind, None, e.message, e.detail)
                for e in self.record.entries
                if e.kind in ("fails", "errors")
            ]
        if not problems and self.record is None:
            problems = [Problem("unfinished", None, "the scenario did not finish", "")]
        return problems


@dataclass
class Feature:
    title: str
    path: str
    scenarios: list = field(default_factory=list)

    def count(self, status):
        return sum(1 for s in self.scenarios if s.status == status)


def count_statuses(scenarios):
    """Scenarios by status."""
    return {
        status: sum(1 for s in scenarios if s.status == status)
        for status in (PASSED, FAILED, ERROR)
    }


def display_path(path, base=None, line=None):
    """A path relative to ``base`` if it is inside, else absolute; with a line.

    Without ``base`` the path is absolute.
    """
    path = os.path.abspath(path)
    shown = path
    if base:
        try:
            shown = os.path.relpath(path, base)
        except ValueError:  # another drive
            shown = path
        if shown == os.pardir or shown.startswith(os.pardir + os.sep):
            shown = path
        shown = shown.replace(os.sep, "/")
    return f"{shown}:{line}" if line else shown


def _titled(problem, base):
    """A problem's text, and where it is written."""
    if not problem.location:
        return problem.text
    path, line = problem.location
    return f"{problem.text} ({display_path(path, base, line)})"


class Report(bdd.Reporter):
    """Collects a run and writes its reports when it finishes.

    ``html_dirs`` each receive an ``index.html``, ``junit_paths`` a JUnit
    XML file. Failing steps name their feature file relative to ``base``,
    absolute without it. The console is ``console.ConsoleReporter``'s.
    """

    def __init__(self, html_dirs=(), junit_paths=(), base=None):
        self.html_dirs = list(html_dirs)
        self.junit_paths = list(junit_paths)
        self.base = base
        self.features = []
        self._current = None  # the running scenario
        self._last = None  # the scenario attach() adds to outside of one

    # -- bdd.Reporter ------------------------------------------------------------

    def feature_start(self, title, path):
        self.features.append(Feature(title, path))

    def scenario_start(self, title, tags, location=None):
        if not self.features:
            self.features.append(Feature("", ""))
        scenario = Scenario.start(title, tags, location)
        self.features[-1].scenarios.append(scenario)
        self._current = self._last = scenario

    def step(self, result, text, message, duration, location=None):
        if self._current:
            self._current.steps.append(Step(result, text, message, duration, location))

    def scenario_end(self, record):
        if not (scenario := self._current):
            return
        scenario.end(record)
        self._current = None

    def interrupted(self, message):
        """End the running scenario, if any, as an error: the run stopped in it."""
        if self._current:
            self._current.steps.append(Step(bdd.ERROR, INTERRUPTED, message, 0.0))
            self.scenario_end(results.current())

    def attach(self, path):
        """Link a file from the running scenario, or else the last one."""
        if scenario := self._current or self._last:
            scenario.attach(path)

    def finish(self):
        """Write every report.

        Each file is written whole or not at all. A report that cannot be
        written is reported on stderr and does not keep the others from
        being written; RuntimeError names them all at the end.
        """
        failed = []
        for directory in self.html_dirs:
            path = os.path.join(directory, "index.html")
            if not _write_file(path, self._html_writer(directory)):
                failed.append(path)
        for path in self.junit_paths:
            if not _write_file(path, self._junit_writer):
                failed.append(path)
        if failed:
            raise RuntimeError(
                f"{len(failed)} reports could not be written: {', '.join(failed)}"
            )

    def _html_writer(self, directory):
        def write(stream):
            stream.write(self.html(directory).encode("utf-8"))

        return write

    def _junit_writer(self, stream):
        ET.ElementTree(self.junit()).write(stream, "utf-8", xml_declaration=True)

    # -- outputs -----------------------------------------------------------------

    def scenarios(self):
        return [s for f in self.features for s in f.scenarios]

    def totals(self):
        return count_statuses(self.scenarios())

    def junit(self):
        """The run as a JUnit XML element tree: a testsuite per feature."""
        root = ET.Element("testsuites", name="GUI tests")
        totals = self.totals()
        _counts(root, len(self.scenarios()), totals[FAILED], totals[ERROR])
        root.set("time", _seconds(sum(s.duration for s in self.scenarios())))
        for feature in self.features:
            suite = ET.SubElement(root, "testsuite", name=_xml(feature.title))
            _counts(
                suite,
                len(feature.scenarios),
                feature.count(FAILED),
                feature.count(ERROR),
            )
            suite.set("time", _seconds(sum(s.duration for s in feature.scenarios)))
            suite.set("file", _xml(feature.path))
            for scenario in feature.scenarios:
                _junit_testcase(suite, feature, scenario, self.base)
        return root

    def html(self, directory):
        """The run as one self-contained HTML page written to ``directory``."""
        totals = self.totals()
        parts = [
            _HTML_HEAD,
            f"<p class=summary>{len(self.scenarios())} scenarios: "
            f"<span class=passed>{totals[PASSED]} passed</span>, "
            f"<span class=failed>{totals[FAILED]} failed</span>, "
            f"<span class=error>{totals[ERROR]} errors</span></p>",
        ]
        for feature in self.features:
            parts.append(
                f"<section><h2>{_h(feature.title)}</h2>"
                f"<p class=path>{_h(feature.path)}</p><table>"
                "<tr><th>Status</th><th>Scenario</th><th>Time</th></tr>"
            )
            for scenario in feature.scenarios:
                parts.append(_html_scenario(scenario, directory, self.base))
            parts.append("</table></section>")
        parts.append("</body></html>\n")
        return "\n".join(parts)


def _write_file(path, write):
    """Write a file through a temporary one beside it; False if that failed.

    ``write`` gets the binary stream. The file is replaced only once it is
    complete, so an interrupted or failed write leaves the previous one.
    """
    # beside the file, so that os.replace does not cross file systems; opened
    # as usual, so that it gets the permissions a new report file would
    temp = f"{path}.{os.getpid()}.tmp"
    created = False
    try:
        os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
        with open(temp, "wb") as stream:
            created = True
            write(stream)
        os.replace(temp, path)
        return True
    except BaseException as exc:  # pylint: disable=broad-exception-caught
        if created:
            _remove_quietly(temp)
        if not isinstance(exc, Exception):
            raise  # interrupted: the run stops here
        print(
            f"[ERROR] cannot write report {path}: {type(exc).__name__}: {exc}",
            file=sys.stderr,
            flush=True,
        )
        return False


def _remove_quietly(path):
    try:
        os.unlink(path)
    except OSError:
        pass


def _screenshots_dir():
    report_dir = os.environ.get("GUI_TEST_REPORT_DIR")
    return (
        os.path.abspath(os.path.join(report_dir, "screenshots")) if report_dir else ""
    )


def _listing(directory):
    """File name -> modification time of the files in a directory."""
    if not directory:
        return {}
    try:
        entries = list(os.scandir(directory))
    except OSError:
        return {}
    return {e.name: e.stat().st_mtime_ns for e in entries if e.is_file()}


def _xml(text):
    return _INVALID_XML.sub("", str(text))


def _seconds(duration):
    return f"{duration:.3f}"


def _counts(element, tests, failures, errors):
    element.set("tests", str(tests))
    element.set("failures", str(failures))
    element.set("errors", str(errors))
    element.set("skipped", "0")


def _junit_testcase(suite, feature, scenario, base):
    case = ET.SubElement(
        suite,
        "testcase",
        name=_xml(scenario.title),
        classname=_xml(feature.title),
        time=_seconds(scenario.duration),
    )
    if scenario.status != PASSED:
        problems = scenario.problems()
        tag = "error" if scenario.status == ERROR else "failure"
        first = problems[0].message if problems else scenario.status
        element = ET.SubElement(case, tag, message=_xml(first), type=scenario.status)
        element.text = _xml(
            "\n\n".join(
                "\n".join(part for part in (f"{title}: {message}", detail) if part)
                for title, message, detail in (
                    (_titled(p, base), p.message, p.detail) for p in problems
                )
            )
        )
    lines = [f"{s.result.upper():7} {s.text}" for s in scenario.steps]
    lines += [f"attachment: {path}" for path in scenario.attachments]
    if scenario.record:
        lines += [
            f"[{e.kind}] {e.message}" + (f": {e.detail}" if e.detail else "")
            for e in scenario.record.entries
            if e.kind not in ("fails", "errors")
        ]
    if lines:
        ET.SubElement(case, "system-out").text = _xml("\n".join(lines))


def _h(text):
    return html.escape(str(text), quote=True)


def _link(path, directory):
    """A link to ``path`` from a page in ``directory``, relative where possible."""
    try:
        target = os.path.relpath(path, os.path.abspath(directory))
    except ValueError:  # another drive
        target = os.path.abspath(path)
    return _h(quote(target.replace(os.sep, "/")))


def _html_scenario(scenario, directory, base):
    status = scenario.status
    tags = " ".join(scenario.tags)
    cells = [
        f"<tr class={status}><td class=status>{status}</td><td>",
        f"<span class=title>{_h(scenario.title)}</span>",
    ]
    if tags:
        cells.append(f" <span class=tags>{_h(tags)}</span>")
    for problem in scenario.problems():
        title, message, detail = _titled(problem, base), problem.message, problem.detail
        cells.append(f"<div class=problem>{_h(title)}<pre>{_h(message)}</pre>")
        if detail:
            cells.append(
                f"<details><summary>details</summary><pre>{_h(detail)}</pre></details>"
            )
        cells.append("</div>")
    if scenario.attachments:
        links = ", ".join(
            f'<a href="{_link(p, directory)}">{_h(os.path.basename(p))}</a>'
            for p in scenario.attachments
        )
        cells.append(f"<div class=attachments>Attachments: {links}</div>")
    if scenario.steps:
        cells.append("<details><summary>steps</summary><ol class=steps>")
        cells.extend(
            f"<li class={s.result}>{_h(s.text)} <small>({s.result}, "
            f"{s.duration:.1f} s)</small></li>"
            for s in scenario.steps
        )
        cells.append("</ol></details>")
    cells.append(f"</td><td class=time>{scenario.duration:.1f} s</td></tr>")
    return "".join(cells)


_HTML_HEAD = """<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8"><title>GUI test report</title>
<style>
body { font-family: sans-serif; margin: 1em 2em; }
table { border-collapse: collapse; width: 100%; }
th, td { border: 1px solid #ccc; padding: 0.3em 0.5em; text-align: left;
         vertical-align: top; }
td.status { font-weight: bold; text-transform: uppercase; width: 5em; }
td.time { white-space: nowrap; width: 5em; }
tr.passed td.status, span.passed, li.passed { color: #1a7f37; }
tr.failed td.status, span.failed, li.failed { color: #cf222e; }
tr.error td.status, span.error, li.error { color: #9a6700; }
li.skipped { color: #777; }
.tags, .path { color: #777; font-size: smaller; }
pre { white-space: pre-wrap; margin: 0.2em 0; background: #f6f8fa; padding: 0.3em; }
</style></head><body>
<h1>GUI test report</h1>"""
