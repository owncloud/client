# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT

import io
import re
import sys

import pytest
from suite_copy import copy_suite

from harness import bdd, results, suite
from harness.console import ConsoleReporter
from harness.tags import TagFilter

FEATURE = """\
Feature: console output

    Scenario: a passing scenario
        Given user "Bob" has been created
        When the step logs "hello from the step"

    Scenario: a breaking scenario
        Given the step logs "::warning::from the log"
        When the step breaks with "100% broken\\n::error::injected"
        Then the remembered value should be "never"
"""

STEPS = """\
@Step('the step logs "|any|"')
def step(context, text):
    test.log(text)


@When('the step breaks with "|any|"')
def step(context, text):
    raise RuntimeError(text.replace('\\\\n', '\\n'))
"""

FEATURE_PATH = "suite/tst_b/test.feature"
BREAKING_STEP = 'When the step breaks with "100% broken\\n::error::injected"'


def _suite(tmp_path, end_hooks=False):
    suite_dir = copy_suite(tmp_path, FEATURE, end_hooks)
    (suite_dir / "shared" / "steps" / "console.py").write_text(STEPS, "utf-8")
    return suite_dir


def _run(tmp_path, mode, end_hooks=False, **kwargs):
    reporter = ConsoleReporter(
        sys.stdout, mode, workspace=str(tmp_path.resolve()), color=False, **kwargs
    )
    suite.run_suite(
        str(_suite(tmp_path, end_hooks)), ["tst_b"], TagFilter([]), reporter=reporter
    )


def _index(lines, pattern):
    """The index of the first line matching a regex."""
    return next(i for i, line in enumerate(lines) if re.search(pattern, line))


# --- pretty ---------------------------------------------------------------------


@pytest.mark.usefixtures("isolated")
def test_pretty_shows_features_scenarios_steps_and_failures(tmp_path, capsys):
    _run(tmp_path, "pretty")
    lines = capsys.readouterr().out.splitlines()
    assert f"Feature: console output  ({FEATURE_PATH})" in lines
    heading = lines.index("Scenario: a passing scenario")
    logged = lines.index("[LOG] hello from the step")
    step = _index(lines, r'^  ✓ When the step logs "hello from the step" +\d+\.\d s$')
    assert heading < logged < step
    assert re.match(r"^✓ a passing scenario  \(\d+\.\d s\)$", lines[step + 1])
    broken = _index(lines, r"^  ✗ When the step breaks with")
    assert lines[broken + 1 : broken + 3] == [
        "      RuntimeError: 100% broken",
        "      ::error::injected",
    ]
    skipped = _index(lines, r'^  - Then the remembered value should be "never"')
    assert lines[skipped].endswith(" skipped")
    result = _index(lines, r"^✗ a breaking scenario  \(error, \d+\.\d s\)$")
    assert lines[result + 1 : result + 4] == [
        f"    {BREAKING_STEP}  ({FEATURE_PATH}:9)",
        "    RuntimeError: 100% broken",
        "    ::error::injected",
    ]
    assert re.match(
        r"^2 scenarios: 1 passed, 0 failed, 1 errors \(\d+\.\d s\)$",
        lines[_index(lines, r"^2 scenarios")],
    )
    assert "5 steps: 3 passed, 0 failed, 1 errors, 1 skipped" in lines
    failing = lines.index("Failing scenarios:")
    assert lines[failing + 1] == f"  {FEATURE_PATH}:7  a breaking scenario"
    assert not any(line.startswith("::") for line in lines[failing:])


# --- github ---------------------------------------------------------------------


def _commands(lines):
    """The workflow commands GitHub runs: (line index, name, properties, data).

    Like the runner, it ignores leading whitespace and runs nothing between
    ``::stop-commands::TOKEN`` and ``::TOKEN::``.
    """
    commands, token = [], None
    for index, line in enumerate(lines):
        line = line.lstrip()
        if token:
            if line == f"::{token}::":
                token = None
            continue
        if not line.startswith("::") or "::" not in line[2:]:
            continue
        head, _, data = line[2:].partition("::")
        name, _, properties = head.partition(" ")
        if name == "stop-commands":
            token = data
            continue
        commands.append((index, name, properties, data))
    return commands, token


def _groups_balanced(lines, commands):
    """Each group closes before the next opens; returns the lines in groups."""
    inside, open_at = set(), None
    for index, name, _, _ in commands:
        if name == "group":
            assert open_at is None, lines[index]
            open_at = index
        elif name == "endgroup":
            assert open_at is not None, lines[index]
            inside.update(range(open_at, index))
            open_at = None
    assert open_at is None
    return inside


@pytest.mark.usefixtures("isolated")
def test_github_groups_annotations_and_escaping(tmp_path, capsys):
    _run(tmp_path, "github", end_hooks=True)
    out = capsys.readouterr().out
    lines = out.splitlines()
    commands, token = _commands(lines)
    assert token is None  # commands run again after the run
    inside = _groups_balanced(lines, commands)
    names = [c[1] for c in commands]
    assert names.count("group") == 3  # two scenarios and the failing end hooks
    assert set(names) == {"group", "endgroup", "error"}
    # the suite's output cannot run commands, whatever it prints
    assert "::warning::from the log" in out
    assert "::error::injected" in out
    results_at = [i for i, line in enumerate(lines) if line[:1] in ("✓", "✗")]
    assert len(results_at) == 3
    assert not inside.intersection(results_at)
    assert lines.index("[LOG] hello from the step") in inside
    errors = [c for c in commands if c[1] == "error"]
    assert [e[2] for e in errors] == [
        f"file={FEATURE_PATH},line=9,title=a breaking scenario",
        f"file={FEATURE_PATH},title=console output%3A OnFeatureEnd",
    ]
    assert errors[0][3] == "RuntimeError: 100%25 broken%0A::error::injected"
    assert errors[1][3] == "RuntimeError: first end hook broke"
    assert not inside.intersection(e[0] for e in errors)


@pytest.mark.usefixtures("isolated")
def test_github_job_summary(tmp_path, capsys):
    summary = tmp_path / "summary.md"
    summary.write_text("earlier step\n", "utf-8")
    _run(tmp_path, "github", step_summary=str(summary))
    capsys.readouterr()
    text = summary.read_text("utf-8")
    assert text.startswith("earlier step\n")
    assert "| Feature | Passed | Failed | Errors | Time |" in text
    assert re.search(r"^\| console output \| 1 \| 0 \| 1 \| \d+\.\d s \|$", text, re.M)
    assert "a breaking scenario" in text
    assert f"<code>{FEATURE_PATH}:9</code>" in text
    assert "RuntimeError: 100% broken\n::error::injected" in text


def test_interrupted_scenario_closes_its_group_and_the_run_is_summed_up():
    stream = io.StringIO()
    reporter = ConsoleReporter(stream, "github", color=False)
    reporter.feature_start("feature", "/x/test.feature")
    reporter.scenario_start("stopped", [], bdd.Location("/x/test.feature", 3))
    reporter.step(bdd.PASSED, "Given a step", "", 0.1, bdd.Location("/x/a", 4))
    reporter.interrupted("interrupted: SIGTERM")
    reporter.finish()
    lines = stream.getvalue().splitlines()
    commands, token = _commands(lines)
    assert token is None
    _groups_balanced(lines, commands)
    assert _index(lines, r"^✗ stopped  \(error, ")
    assert "    interrupted: SIGTERM" in lines
    assert _index(lines, r"^1 scenarios: 0 passed, 0 failed, 1 errors")
    results.reset()


# --- marks and colour -------------------------------------------------------------


def _one_of_each(reporter):
    reporter.feature_start("Fünf", "/x/test.feature")
    reporter.scenario_start("mixed", [])
    reporter.step(bdd.PASSED, "Given it works", "", 0.1)
    reporter.step(bdd.FAILED, "Then it fails", "no", 0.2)
    reporter.step(bdd.SKIPPED, "And it is skipped", "", 0.0)
    record = results.begin_scenario("mixed")
    results.record("fails", "no")
    reporter.scenario_end(record)
    reporter.finish()
    results.reset()


def test_ascii_marks_where_the_stream_cannot_encode_others():
    raw = io.BytesIO()
    stream = io.TextIOWrapper(raw, encoding="ascii")
    _one_of_each(ConsoleReporter(stream, "pretty", color=False))
    stream.flush()
    text = raw.getvalue().decode("ascii")
    assert "  + Given it works" in text
    assert "  x Then it fails" in text
    assert "  - And it is skipped" in text
    assert "Feature: F?nf" in text


@pytest.mark.parametrize(
    "env, colored",
    [
        ({}, False),
        ({"FORCE_COLOR": "1"}, True),
        ({"FORCE_COLOR": "1", "NO_COLOR": "1"}, False),
    ],
)
def test_colour_follows_force_color_and_no_color(env, colored, monkeypatch):
    for name, value in env.items():
        monkeypatch.setenv(name, value)
    stream = io.StringIO()
    _one_of_each(ConsoleReporter(stream, "pretty"))
    assert ("\x1b[" in stream.getvalue()) == colored
