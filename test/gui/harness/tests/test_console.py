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


@pytest.fixture(autouse=True)
def fresh_results():
    """Forget the results a test records, also when it fails."""
    yield
    results.reset()


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
        f"    {BREAKING_STEP} ({FEATURE_PATH}:9)",
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

    Like the runner, it splits lines at CR too and ignores what surrounds a
    line: whitespace, a byte order mark. It knows both forms, ``::name::``
    and the older ``##[name]``.
    """
    commands = []
    for index, line in enumerate(lines):
        for part in re.split("\r\n|\r|\n", line):
            part = part.strip().lstrip("\ufeff")
            if part.startswith("##["):
                commands.append((index, "legacy", "", part))
            if not part.startswith("::") or "::" not in part[2:]:
                continue
            head, _, data = part[2:].partition("::")
            name, _, properties = head.partition(" ")
            commands.append((index, name, properties, data))
    return commands


def _groups_balanced(lines, commands):
    """Each group closes before the next opens; returns {group index: lines}."""
    groups, open_at = {}, None
    for index, name, _, _ in commands:
        if name == "group":
            assert open_at is None, lines[index]
            open_at = index
        elif name == "endgroup":
            assert open_at is not None, lines[index]
            groups[open_at] = lines[open_at + 1 : index]
            open_at = None
    assert open_at is None
    return groups


def _swapped():
    """Whether sys.stdout or sys.stderr is the reporter's capture."""
    return any(
        type(stream).__module__ == "harness.console"
        for stream in (sys.stdout, sys.stderr)
    )


@pytest.fixture(autouse=True)
def real_streams():
    """Fails a test that leaves the streams swapped, and puts them back."""
    yield
    if swapped := _swapped():
        sys.stdout, sys.stderr = sys.__stdout__, sys.__stderr__
    assert not swapped, "the reporter left sys.stdout or sys.stderr swapped"


@pytest.mark.usefixtures("isolated")
def test_github_prints_each_scenario_as_one_block(tmp_path, capsys):
    _run(tmp_path, "github", end_hooks=True)
    assert not _swapped()  # also after the failing hooks
    out = capsys.readouterr().out
    lines = out.splitlines()
    assert "stop-commands" not in out
    commands = _commands(lines)
    groups = _groups_balanced(lines, commands)
    assert {c[1] for c in commands} == {"group", "endgroup", "error"}
    # the passing scenario: one group, its result in the title
    # the failing end hooks' tracebacks are setup output before their block
    passed, setup = [c for c in commands if c[1] == "group"]
    assert setup[3] == f"setup output ({len(groups[setup[0]])} lines)"
    assert "RuntimeError: second end hook broke" in groups[setup[0]]
    assert re.fullmatch(r"✓ a passing scenario  \(\d+\.\d s\)", passed[3])
    inside = groups[passed[0]]
    assert inside[0].startswith('  ✓ Given user "Bob" has been created')
    assert "  [LOG] hello from the step" in inside
    assert not any(line.startswith("✓") for line in lines)
    # the breaking scenario: not grouped, its steps, output and failure
    title = _index(lines, r"^✗ a breaking scenario  \(error, \d+\.\d s\)$")
    broken = _index(lines, r"^  ✗ When the step breaks with")
    # a command only at the start of a line
    assert title < lines.index("  [LOG] ::warning::from the log") < broken
    assert lines[broken + 1 : broken + 4] == [
        "      RuntimeError: 100% broken",
        "      : :error::injected",
        f"      ({FEATURE_PATH}:9)",
    ]
    assert re.match(
        r'^  - Then the remembered value should be "never" +skipped$', lines[broken + 4]
    )
    # the traceback the runner printed, among the scenario's output
    assert "  : :error::injected" in lines[title:broken]
    errors = [c for c in commands if c[1] == "error"]
    assert [e[2] for e in errors] == [
        f"file={FEATURE_PATH},line=9,title=a breaking scenario",
        f"file={FEATURE_PATH},title=console output%3A OnFeatureEnd",
    ]
    assert errors[0][3] == "RuntimeError: 100%25 broken%0A::error::injected"
    assert errors[1][3] == "RuntimeError: first end hook broke"
    assert broken < errors[0][0] < _index(lines, r"^✗ console output: OnFeatureEnd ")
    assert not any(
        e[0] in range(g, g + len(v) + 2) for e in errors for g, v in groups.items()
    )


def _github(stream=None, **kwargs):
    return ConsoleReporter(stream or io.StringIO(), "github", color=False, **kwargs)


def test_github_aligns_step_times_within_a_scenario():
    reporter = _github()
    reporter.feature_start("feature", "/x/test.feature")
    reporter.scenario_start("aligned", [])
    reporter.step(bdd.PASSED, "Given a", "", 0.1)
    reporter.step(bdd.PASSED, ("When a step with a longer text " * 3).strip(), "", 12.3)
    reporter.step(bdd.SKIPPED, "Then b", "", 0.0)
    reporter.scenario_end(results.begin_scenario("aligned"))
    reporter.finish()
    lines = reporter.stream.getvalue().splitlines()
    steps = [line for line in lines if line.startswith("  ")][:3]
    columns = {re.search(r"(?<=  )\S+ s$|(?<=  )skipped$", s).start() for s in steps}
    assert len(columns) == 1
    assert re.search(r"text  12\.3 s$", steps[1])  # the longest: two spaces


def test_github_neutralises_workflow_commands_in_the_output():
    reporter = _github()
    reporter.feature_start("feature", "/x/test.feature")
    reporter.scenario_start("printing", [])
    printed = [
        "::warning::x",
        "  ::warning::indented",
        "\t::warning::tab",
        "\ufeff::warning::bom",
        "\x1b[31m::warning::colour",
        "progress\r::warning::after cr",
        "##[warning]legacy",
        "\u00a0\u200b::warning::other blanks",
    ]
    for text in printed:
        print(text)
    print("::error::on stderr", file=sys.stderr)
    sys.stdout.write("::warning::no newline")
    reporter.step(bdd.FAILED, "Then it fails", "::error::in a message", 0.1)
    record = results.begin_scenario("printing")
    results.record("fails", "::error::in a message")
    reporter.scenario_end(record)
    reporter.finish()
    lines = reporter.stream.getvalue().splitlines()
    assert [c[1] for c in _commands(lines)] == ["error"]
    assert lines[_index(lines, "warning::x$") :][:10] == [
        "  : :warning::x",
        "    : :warning::indented",
        "  \t: :warning::tab",
        "  \ufeff: :warning::bom",
        "  \x1b[31m: :warning::colour",
        "  progress",
        "  : :warning::after cr",
        "  # #[warning]legacy",
        "  \u00a0\u200b: :warning::other blanks",
        "  : :error::on stderr",
    ]
    assert "  : :warning::no newline" in lines
    assert "      : :error::in a message" in lines


@pytest.mark.usefixtures("isolated")
def test_github_setup_output_is_a_group_under_the_feature(tmp_path, capsys):
    suite_dir = _suite(tmp_path)
    test_py = suite_dir / "tst_b" / "test.py"
    test_py.write_text('print("setting up tst_b")\n' + test_py.read_text("utf-8"))
    reporter = _github(sys.stdout, workspace=str(tmp_path.resolve()))
    suite.run_suite(
        str(suite_dir), ["tst_a", "tst_b"], TagFilter([]), reporter=reporter
    )
    lines = capsys.readouterr().out.splitlines()
    heading = lines.index(f"Feature: console output  ({FEATURE_PATH})")
    assert lines[heading + 1 : heading + 4] == [
        "::group::setup output (1 line)",
        "setting up tst_b",
        "::endgroup::",
    ]
    assert lines.count("setting up tst_b") == 1


def test_github_interrupted_scenario_prints_its_output():
    reporter = _github()
    reporter.feature_start("feature", "/x/test.feature")
    reporter.scenario_start("stopped", [], bdd.Location("/x/test.feature", 3))
    reporter.step(bdd.PASSED, "Given a step", "", 0.1, bdd.Location("/x/a", 4))
    print("[LOG] before the stop")
    reporter.interrupted("interrupted: SIGTERM")
    assert not _swapped()
    reporter.finish()
    lines = reporter.stream.getvalue().splitlines()
    _groups_balanced(lines, _commands(lines))
    title = _index(lines, r"^✗ stopped  \(interrupted, ")
    assert lines.index("  [LOG] before the stop") > title
    assert "      interrupted: SIGTERM" in lines
    assert _index(lines, r"^::error .*::interrupted: SIGTERM$")
    assert _index(lines, r"^1 scenarios: 0 passed, 0 failed, 1 errors")


INTERRUPTED_FEATURE = """\
Feature: console output

    Scenario: an interrupted scenario
        Given the step logs "before the interruption"
        When the step is interrupted
"""

INTERRUPTING_STEP = """

@When('the step is interrupted')
def step(context):
    raise KeyboardInterrupt
"""


@pytest.mark.usefixtures("isolated")
def test_github_run_interrupted_mid_scenario(tmp_path, capsys):
    suite_dir = copy_suite(tmp_path, INTERRUPTED_FEATURE)
    (suite_dir / "shared" / "steps" / "console.py").write_text(
        STEPS + INTERRUPTING_STEP, "utf-8"
    )
    reporter = _github(sys.stdout, workspace=str(tmp_path.resolve()))
    with pytest.raises(KeyboardInterrupt):
        suite.run_suite(str(suite_dir), ["tst_b"], TagFilter([]), reporter=reporter)
    assert not _swapped()
    captured = capsys.readouterr()
    lines = captured.out.splitlines()
    title = _index(lines, r"^✗ an interrupted scenario  \(interrupted, ")
    assert lines.index("  [LOG] before the interruption") > title
    assert "  [ERROR] interrupted: KeyboardInterrupt" in lines
    assert not captured.err


@pytest.mark.usefixtures("isolated")
def test_github_reporter_failure_restores_the_streams(tmp_path, capsys):
    def broken(*_args):
        raise RuntimeError("the reporter broke")

    reporter = _github(sys.stdout, workspace=str(tmp_path.resolve()))
    reporter._block = broken  # pylint: disable=protected-access
    with pytest.raises(RuntimeError, match="the reporter broke"):
        suite.run_suite(
            str(_suite(tmp_path)),
            ["tst_b"],
            TagFilter([]),
            reporter=bdd.Reporters([reporter]),
        )
    assert not _swapped()
    captured = capsys.readouterr()
    assert "[LOG] hello from the step" in captured.out
    assert "the reporter broke" in captured.err


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


def test_pretty_does_not_capture():
    reporter = ConsoleReporter(io.StringIO(), "pretty", color=False)
    reporter.feature_start("feature", "/x/test.feature")
    reporter.scenario_start("live", [])
    assert not _swapped()
    reporter.scenario_end(results.begin_scenario("live"))
    reporter.finish()


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


# --- what a failing scenario lists --------------------------------------------


def test_failing_scenario_lists_its_new_screenshots_and_logs(tmp_path, monkeypatch):
    shots = tmp_path / "reports" / "screenshots"
    shots.mkdir(parents=True)
    (shots / "old.png").write_bytes(b"old")
    monkeypatch.setenv("GUI_TEST_REPORT_DIR", str(tmp_path / "reports"))
    monkeypatch.chdir(tmp_path)
    stream = io.StringIO()
    reporter = ConsoleReporter(stream, color=False)
    reporter.feature_start("feature", "/x/test.feature")
    record = results.begin_scenario("broken")
    reporter.scenario_start("broken", [])
    (shots / "broken.png").write_bytes(b"png")
    reporter.step(bdd.ERROR, "Then it breaks", "RuntimeError: no", 0.1)
    reporter.attach("client.log")  # relative, as given
    results.record("errors", "RuntimeError: no")
    reporter.scenario_end(record)
    lines = stream.getvalue().splitlines()
    assert f"    attachment: {tmp_path / 'client.log'}" in lines
    assert f"    attachment: {shots / 'broken.png'}" in lines
    assert not any("old.png" in line for line in lines)


def test_scenario_left_open_is_unfinished_not_the_next_record(tmp_path):
    stream = io.StringIO()
    reporter = ConsoleReporter(stream, workspace=str(tmp_path), color=False)
    reporter.feature_start("feature", str(tmp_path / "test.feature"))
    results.begin_scenario("left open")
    reporter.scenario_start("left open", [])
    reporter.step(bdd.PASSED, "Given a step", "", 0.1)
    # the runner has moved on, as to a feature hook's record
    results.begin_scenario("feature: OnFeatureEnd")
    results.record("errors", "RuntimeError: the end hook broke")
    reporter.feature_start("next", str(tmp_path / "next.feature"))
    reporter.finish()
    lines = stream.getvalue().splitlines()
    result = _index(lines, r"^✗ left open  \(error, ")
    assert lines[result + 1 : result + 3] == [
        "    unfinished",
        "    the scenario did not finish",
    ]
    assert not any("end hook broke" in line for line in lines)
