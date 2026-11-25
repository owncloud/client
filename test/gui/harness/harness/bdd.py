# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Gherkin features run with Squish's BDD semantics.

Steps match by their text alone, whatever the keyword. Hooks run in the
order Squish runs them: OnFeatureStart, then per scenario OnScenarioStart,
OnStepStart and OnStepEnd around every step, OnScenarioEnd; OnFeatureEnd
last. Hooks of one kind run in the order they were defined. Before a
scenario's OnScenarioStart hooks the runner's ``before_scenario`` runs, and
after its OnScenarioEnd hooks ``after_scenario``; the suite links the logs
of the applications started in the scenario there and lets the
applications release their handles.
"""

import re
import sys
import time
import traceback
from dataclasses import dataclass
from typing import NamedTuple

from gherkin.parser import Parser
from gherkin.pickles.compiler import Compiler
from gherkin.token_matcher import TokenMatcher

from harness import results
from harness.tags import TagFilter

# Step results passed to Reporter.step().
PASSED = "passed"
FAILED = "failed"
ERROR = "error"
SKIPPED = "skipped"

HOOK_NAMES = (
    "OnFeatureStart",
    "OnFeatureEnd",
    "OnScenarioStart",
    "OnScenarioEnd",
    "OnStepStart",
    "OnStepEnd",
)
STEP_KEYWORDS = ("Given", "When", "Then", "Step")
# The step a failing Runner.after_scenario is reported as.
AFTER_SCENARIO_STEP = "release handles"

# Placeholder name -> (regex group, converter for the argument).
_PLACEHOLDERS = {  # pylint: disable=consider-using-namedtuple-or-dataclass
    "any": (r"(.+?)", str),
    "word": (r"(\w+)", str),
    "integer": (r"([-+]?\d+)", int),
}
_PLACEHOLDER_RE = re.compile(r"\|(any|word|integer)\|")


class _SquishTableMatcher(TokenMatcher):
    """Gherkin's lexer, with table cells split the way Squish passes them.

    Cucumber unescapes ``\\\\``, ``\\n`` and ``\\|`` in table cells; Squish
    passes a backslash through as written (``a\\\\a.txt`` stays two
    backslashes). Splitting the row here, at the lexer's table-row token,
    covers data tables and Examples tables alike, so ``<x>`` substitutes the
    cell as written, and it leaves doc strings and step text alone.
    """

    def match_TableRow(self, token):
        if not token.line.startswith("|"):
            return False
        text = token.line.get_line_text()  # without the indent
        cells = []
        for cell, column in _split_table_row(text.rstrip()):
            stripped = cell.lstrip()
            cells.append(
                {
                    "column": token.line.indent + column + len(cell) - len(stripped),
                    "text": stripped.rstrip(),
                }
            )
        self._set_token_matched(token, "TableRow", items=cells)
        return True


def _split_table_row(row):
    """Yield (cell text, 1-based column) for each cell between pipes.

    A backslash is literal, except that ``\\|`` is a pipe inside a cell. That
    is an assumption: Squish's handling of ``\\|`` was not observed, but it
    is the only way to write a pipe into a cell. Read left to right, ``\\\\``
    is a pair of literal backslashes (kept as both), so ``C:\\\\|`` ends the
    cell after ``C:\\\\``. Text before the first and after the last pipe is
    not a cell.
    """
    cell, start, index = "", None, 0
    while index < len(row):
        char = row[index]
        if char == "\\" and (escaped := row[index + 1 : index + 2]) in ("\\", "|"):
            cell += "\\\\" if escaped == "\\" else "|"
            index += 2
            continue
        if char == "|":
            if start is not None:
                yield cell, start
            cell, start = "", index + 2
        else:
            cell += char
        index += 1


class UndefinedStep(LookupError):
    pass


class AmbiguousStep(LookupError):
    pass


@dataclass
class _Definition:
    pattern: str
    regexp: bool
    regex: re.Pattern
    converters: tuple
    fn: object

    def args(self, match):
        if self.regexp:
            return match.groups()
        return tuple(
            convert(value) for convert, value in zip(self.converters, match.groups())
        )


def compile_pattern(pattern, regexp):
    """A step pattern as a regex matched against the whole step text.

    Squish placeholders become groups; the rest of the pattern is literal.
    With ``regexp`` the pattern is a regex already. Returns the compiled
    regex and one converter per placeholder.
    """
    if regexp:
        return re.compile(pattern), ()
    regex, converters = [], []
    for index, part in enumerate(_PLACEHOLDER_RE.split(pattern)):
        if index % 2:
            group, convert = _PLACEHOLDERS[part]
            regex.append(group)
            converters.append(convert)
        else:
            regex.append(re.escape(part))
    return re.compile("".join(regex)), tuple(converters)


class StepRegistry:
    """The step definitions of one test case."""

    def __init__(self):
        self._definitions = []

    def add(self, pattern, fn, regexp=False):
        """Add a definition; a pattern that is already defined keeps the first.

        Squish lets the step directories collected first take precedence, so
        a test case's own ``steps`` can replace a shared step.
        """
        if any(d.pattern == pattern and d.regexp == regexp for d in self._definitions):
            return
        regex, converters = compile_pattern(pattern, regexp)
        self._definitions.append(_Definition(pattern, regexp, regex, converters, fn))

    def match(self, text):
        """The function for a step text, and its arguments."""
        found = [
            (definition, match)
            for definition in self._definitions
            if (match := definition.regex.fullmatch(text))
        ]
        if not found:
            raise UndefinedStep(f'Undefined step: "{text}"')
        if len(found) > 1:
            patterns = ", ".join(repr(d.pattern) for d, _ in found)
            raise AmbiguousStep(f'Ambiguous step: "{text}" matches {patterns}')
        definition, match = found[0]
        return definition.fn, definition.args(match)


class Location(NamedTuple):
    """Where a scenario or step is written: a feature file and a 1-based line."""

    path: str
    line: int

    def __str__(self):
        return f"{self.path}:{self.line}"


class Context:  # pylint: disable=too-few-public-methods
    """The ``context`` that steps and hooks get.

    ``title`` is the feature's name in feature hooks and the scenario's name
    everywhere else. ``table`` is the step's data table as rows of strings,
    header row included, and ``multiLineText`` its doc string as a list of
    lines; both are None for a step without one. ``userData`` belongs to the
    suite and lasts for one scenario.
    """

    # Squish's attribute names, used by the suite.
    # pylint: disable=invalid-name
    def __init__(self, title):
        self.title = title
        self.table = None
        self.multiLineText = None
        self.userData = None


class Reporter:
    """What the runner tells reports about; every method does nothing here.

    ``feature_start(title, path)``, then per scenario ``scenario_start(title,
    tags, location)``, one ``step(result, text, message, duration,
    location)`` per step and ``scenario_end(record)`` with the scenario's
    ``results.Results``; ``finish()`` once at the end. ``result`` is one of
    PASSED, FAILED, ERROR or SKIPPED, ``text`` starts with the keyword,
    ``duration`` is in seconds. ``location`` is a ``Location``: the
    scenario's line, or for a Scenario Outline the line of its Examples row,
    and the step's line. A failing hook is reported as a step named after
    the hook. Failing feature hooks of one kind are reported as one scenario
    of their own, with a step per failure; a failing ``test.py`` as a
    feature named after its test case, with one such scenario. Those have no
    location: it is None.
    ``attach(path)`` links a file to the running scenario; the suite calls it
    from ``after_scenario`` for the logs of the applications started in it.
    ``interrupted(message)`` comes before ``finish()`` when the run stops
    in the middle, as on KeyboardInterrupt or SIGTERM; a running scenario
    then ends as an error.
    """

    def feature_start(self, title, path):
        pass

    def scenario_start(self, title, tags, location=None):
        pass

    def step(self, result, text, message, duration, location=None):
        pass

    def scenario_end(self, record):
        pass

    def attach(self, path):
        pass

    def interrupted(self, message):
        pass

    def finish(self):
        pass


class Reporters(Reporter):
    """Several reporters as one: each call goes to every one of them, in order.

    A reporter that raises does not keep the others from getting the call.
    Its exception is reported on stderr, unless it is an interruption such
    as KeyboardInterrupt, and once all have been called the first exception
    is raised again; the first interruption, if there was one.
    """

    def __init__(self, reporters):
        self.reporters = list(reporters)

    def _forward(self, method, *args, **kwargs):
        first = None
        for reporter in self.reporters:
            try:
                getattr(reporter, method)(*args, **kwargs)
            except BaseException as exc:  # pylint: disable=broad-exception-caught
                if isinstance(exc, Exception):
                    print(
                        f"[ERROR] reporter {type(reporter).__name__}.{method} failed: "
                        f"{type(exc).__name__}: {exc}",
                        file=sys.stderr,
                        flush=True,
                    )
                # an interruption wins over a failure, so that the run stops
                if first is None or (
                    isinstance(first, Exception) and not isinstance(exc, Exception)
                ):
                    first = exc
        if first is not None:
            raise first

    def feature_start(self, title, path):
        self._forward("feature_start", title, path)

    def scenario_start(self, title, tags, location=None):
        self._forward("scenario_start", title, tags, location)

    def step(self, result, text, message, duration, location=None):
        self._forward("step", result, text, message, duration, location)

    def scenario_end(self, record):
        self._forward("scenario_end", record)

    def attach(self, path):
        self._forward("attach", path)

    def interrupted(self, message):
        self._forward("interrupted", message)

    def finish(self):
        self._forward("finish")


def outcome(exc, fails_before):
    """Classify an exception from suite code and record it.

    With ``testSettings.throwOnFailure`` a failed verification raises an
    AssertionError after recording the fail; that stays a fail. Any other
    exception is an error.
    """
    if isinstance(exc, AssertionError) and results.run_count("fails") > fails_before:
        return FAILED, str(exc)
    message = f"{type(exc).__name__}: {exc}"
    detail = "".join(traceback.format_exception(type(exc), exc, exc.__traceback__))
    results.record("errors", message, detail)
    print(f"[ERROR] {message}\n{detail}", end="", flush=True)
    return ERROR, message


class Runner:  # pylint: disable=too-many-instance-attributes
    """Runs feature files against the step definitions and hooks of a test case."""

    def __init__(self, tag_filter=None, reporter=None, abort_on_fail=False):
        self.tag_filter = tag_filter or TagFilter([])
        self.reporter = reporter or Reporter()
        self.abort_on_fail = abort_on_fail
        # Called before each scenario's OnScenarioStart hooks; must not raise.
        self.before_scenario = _nothing
        # Called after each scenario's OnScenarioEnd hooks; an exception is
        # an error of the scenario.
        self.after_scenario = _nothing
        self.aborted = False
        self.registry = StepRegistry()
        self.hooks = {name: [] for name in HOOK_NAMES}

    def reset_definitions(self):
        """Forget steps and hooks, as at the start of a test case."""
        self.registry = StepRegistry()
        self.hooks = {name: [] for name in HOOK_NAMES}

    def builtins(self):
        """The step and hook decorators, by their Squish names."""
        names = {keyword: self._step_decorator for keyword in STEP_KEYWORDS}
        for name in HOOK_NAMES:
            names[name] = self._hook_decorator(name)
        return names

    def _step_decorator(self, pattern, regexp=False):
        def register(fn):
            self.registry.add(pattern, fn, regexp)
            return fn

        return register

    def _hook_decorator(self, name):
        def register(fn):
            self.hooks[name].append(fn)
            return fn

        register.__name__ = name
        return register

    def report_problems(self, title, problems):
        """Report failures outside any scenario as one scenario of their own.

        ``problems`` are (result, message, duration) tuples, each reported as
        a step; they all belong to the current ``results`` record.
        """
        self.reporter.scenario_start(title, [])
        for result, message, duration in problems:
            self.reporter.step(result, title, message, duration)
        self.reporter.scenario_end(results.current())
        if self.abort_on_fail:
            self.aborted = True

    # -- running ---------------------------------------------------------------

    def run_feature_file(self, path):
        """Run the scenarios of a feature file that the tag filter accepts."""
        with open(path, encoding="utf-8") as stream:
            document = Parser().parse(stream.read(), _SquishTableMatcher())
        document["uri"] = path
        feature = document.get("feature")
        if not feature or self.aborted:
            return
        scenarios = []
        for pickle in Compiler().compile(document):
            tags = [tag["name"] for tag in pickle["tags"]]
            if self.tag_filter.accepts(tags):
                scenarios.append((pickle, tags))
        if not scenarios:
            return
        nodes = _ast_nodes(feature)
        title = feature["name"]
        context = Context(title)
        self.reporter.feature_start(title, path)
        if self._run_feature_hooks("OnFeatureStart", context):
            lines = _Lines(path, nodes)
            for pickle, tags in scenarios:
                failed = self._run_scenario(pickle, tags, lines)
                if failed and self.abort_on_fail:
                    self.aborted = True
                    break
        self._run_feature_hooks("OnFeatureEnd", context)

    def _run_feature_hooks(self, name, context):
        title = f"{context.title}: {name}"
        results.begin_scenario(title)
        stop_on_error = name.endswith("Start")
        if problems := self._run_hooks(name, context, stop_on_error):
            self.report_problems(title, problems)
            return False
        return True

    def _run_hooks(self, name, context, stop_on_error):
        """Run the hooks of one kind; returns (result, message, duration) per failure.

        Start hooks stop at the first failure. End hooks clean up, so all of
        them run.
        """
        problems = []
        for hook in self.hooks[name]:
            start = time.monotonic()
            fails_before = results.run_count("fails")
            try:
                hook(context)
            except Exception as exc:  # pylint: disable=broad-exception-caught
                result, message = outcome(exc, fails_before)
                problems.append((result, message, time.monotonic() - start))
                if stop_on_error:
                    break
        return problems

    def _report_hooks(self, name, problems):
        for result, message, duration in problems:
            self.reporter.step(result, f"{name} hook", message, duration)

    def _run_scenario(self, pickle, tags, lines):
        """Run one scenario; returns whether it failed.

        After a step that raised (an error, or a fail with throwOnFailure)
        the remaining steps are skipped. OnScenarioEnd always runs.
        """
        title = pickle["name"]
        record = results.begin_scenario(title)
        # an outline's pickle names the Examples row last
        self.reporter.scenario_start(title, tags, lines.at(pickle["astNodeIds"][-1]))
        self.before_scenario()
        context = Context(title)
        problems = self._run_hooks("OnScenarioStart", context, stop_on_error=True)
        self._report_hooks("OnScenarioStart", problems)
        proceed = not problems
        for step in pickle["steps"]:
            node_id = step["astNodeIds"][0]  # the step; an Examples row follows
            keyword = lines.keyword(node_id)
            text = f"{keyword} {step['text']}" if keyword else step["text"]
            location = lines.at(node_id)
            if proceed:
                proceed = self._run_step(step, text, context, location)
            else:
                self.reporter.step(SKIPPED, text, "", 0.0, location)
        context.table = context.multiLineText = None
        self._report_hooks(
            "OnScenarioEnd",
            self._run_hooks("OnScenarioEnd", context, stop_on_error=False),
        )
        self._run_after_scenario()
        self.reporter.scenario_end(record)
        return record.failed

    def _run_after_scenario(self):
        start = time.monotonic()
        fails_before = results.run_count("fails")
        try:
            self.after_scenario()
        except Exception as exc:  # pylint: disable=broad-exception-caught
            result, message = outcome(exc, fails_before)
            self.reporter.step(
                result, AFTER_SCENARIO_STEP, message, time.monotonic() - start
            )

    def _run_step(self, step, text, context, location):
        """Run one step and its step hooks; returns whether the next step runs."""
        argument = step.get("argument", {})
        context.table = None
        context.multiLineText = None
        if "dataTable" in argument:
            context.table = [
                [cell["value"] for cell in row["cells"]]
                for row in argument["dataTable"]["rows"]
            ]
        if "docString" in argument:
            context.multiLineText = argument["docString"]["content"].split("\n")
        problems = self._run_hooks("OnStepStart", context, stop_on_error=True)
        self._report_hooks("OnStepStart", problems)
        if problems:
            self.reporter.step(SKIPPED, text, "", 0.0, location)
            return False
        start = time.monotonic()
        result, message, proceed = self._execute(step["text"], context)
        self.reporter.step(result, text, message, time.monotonic() - start, location)
        problems = self._run_hooks("OnStepEnd", context, stop_on_error=False)
        self._report_hooks("OnStepEnd", problems)
        return proceed and not problems

    def _execute(self, text, context):
        """Run a step's function: (result, message, whether to go on)."""
        try:
            fn, args = self.registry.match(text)
        except (UndefinedStep, AmbiguousStep) as exc:
            results.record("errors", str(exc))
            print(f"[ERROR] {exc}", flush=True)
            return ERROR, str(exc), False
        record = results.current()
        seen = len(record.entries)
        fails_before = results.run_count("fails")
        try:
            fn(context, *args)
        except Exception as exc:  # pylint: disable=broad-exception-caught
            result, message = outcome(exc, fails_before)
            return result, message, False
        if fails := [e for e in record.entries[seen:] if e.kind == "fails"]:
            # a failed verification without throwOnFailure: the scenario goes on
            message = "; ".join(
                f"{e.message}: {e.detail}" if e.detail else e.message for e in fails
            )
            return FAILED, message, True
        return PASSED, "", True


def _nothing():
    pass


class _Lines:
    """The AST nodes of one feature file, looked up by the ids pickles carry."""

    def __init__(self, path, nodes):
        self.path = path
        self.nodes = nodes

    def at(self, node_id):
        node = self.nodes.get(node_id)
        return Location(self.path, node["location"]["line"]) if node else None

    def keyword(self, node_id):
        return self.nodes.get(node_id, {}).get("keyword", "").strip()


def _ast_nodes(node):
    """Map the ids of a feature's (or rule's) scenarios, steps and Examples
    rows to their AST nodes."""
    nodes = {}
    for child in node.get("children", []):
        for part in (child.get("background"), child.get("scenario")):
            if not part:
                continue
            nodes[part["id"]] = part
            for step in part.get("steps", []):
                nodes[step["id"]] = step
            for examples in part.get("examples", []):
                for row in examples.get("tableBody", []):
                    nodes[row["id"]] = row
        if "rule" in child:
            nodes.update(_ast_nodes(child["rule"]))
    return nodes
