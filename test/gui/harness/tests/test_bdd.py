# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT

import builtins
import errno
import importlib
import json
import os
import shutil
import sys
import types

import pytest
from suite_copy import SUITE_MIN, copy_suite

from fake_probe import FakeError, FakeProbe
from harness import __main__ as cli
from harness import aut, bdd, results, suite
from harness.tags import TagFilter


class RecordingReporter:
    def __init__(self):
        self.calls = []
        self.locations = []  # (title or step text, location) in call order

    def feature_start(self, title, path):
        self.calls.append(("feature_start", title, path))

    def scenario_start(self, title, tags, location=None):
        self.calls.append(("scenario_start", title, tags))
        self.locations.append((title, location))

    def step(self, result, text, message, duration, location=None):
        assert duration >= 0
        self.calls.append(("step", result, text, message))
        self.locations.append((text, location))

    def scenario_end(self, record):
        self.calls.append(("scenario_end", record.title, record.failed))

    def attach(self, path):
        self.calls.append(("attach", path))

    def interrupted(self, message):
        self.calls.append(("interrupted", message))

    def finish(self):
        self.calls.append(("finish",))

    def steps(self, result=None):
        return [c for c in self.calls if c[0] == "step" and result in (None, c[1])]


def _events(path):
    if not path.exists():
        return []
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()]


def _run(suite_dir=SUITE_MIN, tags=(), abort_on_fail=False, testcases=None):
    reporter = RecordingReporter()
    code = suite.run_suite(
        str(suite_dir), testcases, TagFilter(list(tags)), abort_on_fail, reporter
    )
    return code, reporter


# --- tags ---------------------------------------------------------------------


def test_tag_filter_clauses_are_anded_and_negated():
    tag_filter = TagFilter(["~@skip", "~@skipOnLinux"])
    assert not tag_filter.accepts({"@skip"})
    assert not tag_filter.accepts({"@smokeTest", "@skipOnLinux"})
    assert tag_filter.accepts({"@smokeTest"})
    assert tag_filter.accepts(set())


def test_tag_filter_comma_is_or():
    tag_filter = TagFilter(["@a,@b"])
    assert tag_filter.accepts({"@b"})
    assert not tag_filter.accepts({"@c"})
    assert TagFilter(["@a,~@b"]).accepts(set())
    assert TagFilter([]).accepts({"@anything"})


def test_tag_filter_rejects_malformed_terms():
    with pytest.raises(ValueError):
        TagFilter(["skip"])
    with pytest.raises(ValueError):
        TagFilter(["@a,"])


# --- step registry ------------------------------------------------------------


def _fn(*_):
    return None


def test_any_placeholder_matches_lazily_and_whole_text():
    registry = bdd.StepRegistry()
    registry.add('user "|any|" creates a file "|any|"', _fn, regexp=False)
    fn, args = registry.match('user "Alice" creates a file "a b.txt"')
    assert fn is _fn
    assert args == ("Alice", "a b.txt")
    with pytest.raises(bdd.UndefinedStep):
        registry.match('user "Alice" creates a file "a b.txt" twice')


def test_word_and_integer_placeholders_and_escaping():
    registry = bdd.StepRegistry()
    registry.add("the word |word| has |integer| letters (really?)", _fn, regexp=False)
    _, args = registry.match("the word notes has -5 letters (really?)")
    assert args == ("notes", -5)
    with pytest.raises(bdd.UndefinedStep):
        registry.match("the word two words has 5 letters (really?)")


def test_regexp_pattern_is_used_as_given_and_anchored():
    registry = bdd.StepRegistry()
    registry.add(
        r'the user waits for (file|folder) "([^"]*)" to be synced', _fn, regexp=True
    )
    registry.add(r'^the user presses the "([^"]*)" key(?:s)?', _fn, regexp=True)
    _, args = registry.match('the user waits for folder "f 1" to be synced')
    assert args == ("folder", "f 1")
    _, args = registry.match('the user presses the "Ctrl+A" keys')
    assert args == ("Ctrl+A",)
    with pytest.raises(bdd.UndefinedStep):
        registry.match('the user waits for file "x" to be synced now')


def test_matching_ignores_the_keyword():
    runner = bdd.Runner(TagFilter([]), None)
    names = runner.builtins()

    @names["When"]("the user does it")
    def step(context):
        return None

    @names["Then"]("it is done")
    def step2(context):
        return None

    assert runner.registry.match("the user does it")[0] is step
    assert runner.registry.match("it is done")[0] is step2


def test_ambiguous_and_duplicate_patterns():
    registry = bdd.StepRegistry()
    registry.add('user "|any|" logs in', _fn, regexp=False)
    registry.add(r'user "(\w+)" logs in', _fn, regexp=True)
    with pytest.raises(bdd.AmbiguousStep) as info:
        registry.match('user "Alice" logs in')
    assert 'user "Alice" logs in' in str(info.value)

    first = bdd.StepRegistry()
    first.add("same text", _fn, regexp=False)
    first.add("same text", print, regexp=False)  # a later definition loses
    assert first.match("same text")[0] is _fn


# --- running a suite ----------------------------------------------------------


def test_hooks_fire_in_squish_order(isolated):
    code, _ = _run(tags=["~@skip"])
    events = _events(isolated)
    assert code == 1
    first = "tables, doc strings and user data"
    order = [e[:2] for e in events if e[0].startswith("On")]
    assert order[0] == ["OnFeatureStart", "minimal suite"]
    assert order[1] == ["OnScenarioStart", first]
    assert order[-1] == ["OnFeatureEnd", "minimal suite"]
    first_scenario = order[1 : order.index(["OnScenarioEnd", first]) + 2]
    steps = 7  # one Background step and six scenario steps
    assert first_scenario[1 : 1 + 2 * steps : 2] == [["OnStepStart", first]] * steps
    assert first_scenario[2 : 2 + 2 * steps : 2] == [["OnStepEnd", first]] * steps
    assert first_scenario[-2:] == [["OnScenarioEnd", first], ["OnScenarioEnd:2", first]]


def test_suite_environment_and_exec_globals(isolated):
    _run(tags=["~@skip"])
    setup = next(e for e in _events(isolated) if e[0] == "setup")
    assert setup == ["setup", "tst_a", "hello from envs.txt", True]
    assert os.environ["SUITE_MIN_GREETING"] == "hello from envs.txt"
    assert builtins.object is object  # Squish's object stays out of builtins


def test_table_doc_string_placeholders_and_user_data(isolated):
    code, reporter = _run(tags=["~@skip", "@smokeTest"])
    events = _events(isolated)
    assert not code
    table = next(e[1] for e in events if e[0] == "table")
    assert table == [["user", "server"], ["Alice", "example.test"]]
    docstring = next(e for e in events if e[0] == "docstring")
    assert docstring == ["docstring", "Alice", "notes.txt", "first line\n  second line"]
    assert ["step", "notes", 5] in events
    assert ["step", "synced file notes.txt"] in events
    assert [c[1] for c in reporter.steps()] == ["passed"] * 7
    assert reporter.steps()[0][2] == 'Given user "Alice" has been created'
    assert reporter.steps()[4][2] == 'And the user remembers "kept"'
    assert results.result_count("passes") == 2


def test_crashing_step_is_an_error_and_the_run_goes_on(isolated):
    code, reporter = _run(tags=["~@skip"])
    events = _events(isolated)
    assert code == 1
    assert results.result_count("errors") == 1
    assert not results.result_count("fails")
    crash = "the application crashes"
    assert ["OnScenarioEnd", crash, True] in events
    assert ["OnScenarioEnd:2", crash] in events
    scenario = reporter.calls.index(("scenario_start", crash, ["@crash"]))
    assert reporter.calls[scenario + 2][:3] == (
        "step",
        "error",
        "When the application crashes",
    )
    assert "the application under test crashed" in reporter.calls[scenario + 2][3]
    assert reporter.calls[scenario + 3][:3] == (
        "step",
        "skipped",
        'Then the remembered value should be "never"',
    )
    assert ("scenario_end", crash, True) in reporter.calls
    # the next scenarios still run, with fresh user data
    assert ["step", "synced folder one"] in events
    assert ["step", "synced folder two"] in events
    assert ["OnScenarioEnd", "syncing after a crash", False] in events
    assert reporter.calls[-1] == ("finish",)


def test_tags_select_scenarios(isolated):
    code, reporter = _run(tags=["~@skip", "~@crash"])
    assert not code
    titles = [c[1] for c in reporter.calls if c[0] == "scenario_start"]
    assert titles == [
        "tables, doc strings and user data",
        "syncing after a crash",
        "syncing after a crash",
    ]
    _run()
    assert ["step", "synced file skipped.txt"] in _events(isolated)


def test_abort_on_fail_stops_after_the_first_failed_scenario(isolated):
    code, reporter = _run(tags=["~@skip"], abort_on_fail=True)
    events = _events(isolated)
    assert code == 1
    titles = [c[1] for c in reporter.calls if c[0] == "scenario_start"]
    assert titles == ["tables, doc strings and user data", "the application crashes"]
    assert ["OnFeatureEnd", "minimal suite"] in events


UNDEFINED = """\
Feature: second feature

    Scenario: an undefined step
        Given user "Bob" has been created
        When the user does something nobody implemented
        Then nothing should be remembered


    Scenario: a failed verification
        Given the user remembers "this"
        Then the remembered value should be "that"
        And nothing should be remembered
"""


def test_undefined_step_and_failed_verification(isolated, tmp_path):
    suite_dir = copy_suite(tmp_path, UNDEFINED)
    code, reporter = _run(suite_dir, tags=["~@skip", "~@crash"], testcases=["tst_b"])
    assert code == 1
    assert results.result_count("errors") == 1
    assert results.result_count("fails") == 1
    undefined = reporter.steps("error")[0]
    assert "the user does something nobody implemented" in undefined[3]
    assert [c[1] for c in reporter.steps()] == [
        "passed",
        "error",
        "skipped",
        "passed",
        "failed",
        "skipped",
    ]
    assert "remembered value" in reporter.steps("failed")[0][3]
    assert ["OnFeatureStart", "second feature"] in _events(isolated)


PASSING = """\
Feature: passing feature

    Scenario: a passing scenario
        Given user "Carol" has been created
"""


def test_test_cases_run_in_order_and_count_results_per_test_case(isolated, tmp_path):
    suite_dir = copy_suite(tmp_path, PASSING)
    code, reporter = _run(suite_dir, tags=["~@skip"])
    assert code == 1  # the crash in tst_a
    features = [c[1] for c in reporter.calls if c[0] == "feature_start"]
    assert features == ["minimal suite", "passing feature"]
    # test.resultCount counts the current test case, as in Squish, so the
    # hooks of tst_b do not see tst_a's error; the run still failed
    assert ["OnScenarioEnd", "a passing scenario", False] in _events(isolated)
    assert not results.result_count("errors")
    assert results.run_count("errors") == 1


def test_namespace_packages_are_imported_afresh_per_test_case(isolated, tmp_path):
    # pkg and pkg.sub have no __init__.py, like helpers and helpers.api in a
    # real suite; their __path__ looks up the parent package in sys.modules
    suite_dir = copy_suite(tmp_path, PASSING)
    code, reporter = _run(suite_dir, tags=["~@skip"])
    assert code == 1  # the crash in tst_a
    features = [c[1] for c in reporter.calls if c[0] == "feature_start"]
    assert features == ["minimal suite", "passing feature"]
    events = _events(isolated)
    # the module's code ran once per test case: each got a fresh module object
    assert [e for e in events if e[0] == "imported"] == [
        ["imported", "pkg.sub.mod"]
    ] * 2
    greeting = "hello from a namespace package"
    assert [e for e in events if e[0] == "hooks"] == [["hooks", greeting]] * 2
    assert ["step", "created Carol", greeting] in events
    assert reporter.calls[-1] == ("finish",)


@pytest.mark.usefixtures("isolated")
def test_modules_that_cannot_be_placed_are_skipped(tmp_path, capsys):
    suite_dir = copy_suite(tmp_path, PASSING)
    sys.path.insert(0, str(suite_dir / "shared" / "scripts"))
    module = importlib.import_module("pkg.sub.mod")

    class Unplaceable(types.ModuleType):  # pylint: disable=too-few-public-methods
        @property
        def __spec__(self):
            raise RuntimeError("no spec today")

    sys.modules["pkg.sub.unplaceable"] = Unplaceable("pkg.sub.unplaceable")
    sys.modules["elsewhere_unplaceable"] = Unplaceable("elsewhere_unplaceable")
    code, _ = _run(suite_dir, testcases=["tst_b"])
    assert not code
    assert sys.modules["pkg.sub.mod"] is not module
    # dropped with its package, though it cannot tell where it lives
    assert "pkg.sub.unplaceable" not in sys.modules
    assert "elsewhere_unplaceable" in sys.modules
    assert "no spec today" in capsys.readouterr().err


def test_error_between_test_cases_is_an_error_and_the_run_goes_on(
    isolated, tmp_path, monkeypatch
):
    suite_dir = copy_suite(tmp_path, PASSING)
    forget = vars(suite.Suite)["_forget_suite_modules"]
    calls = []

    def forget_once_broken(self):
        calls.append(self.testcase_dir.name)
        if len(calls) == 1:
            raise RuntimeError("could not forget modules")
        forget(self)

    monkeypatch.setattr(suite.Suite, "_forget_suite_modules", forget_once_broken)
    code, reporter = _run(suite_dir, tags=["~@skip"])
    assert code == 1
    assert calls == ["tst_a", "tst_b"]
    errors = reporter.steps("error")
    assert [e[2] for e in errors] == ["tst_a: setup"]
    assert "could not forget modules" in errors[0][3]
    features = [c[1] for c in reporter.calls if c[0] == "feature_start"]
    assert features == ["tst_a", "passing feature"]
    assert ["OnScenarioEnd", "a passing scenario", False] in _events(isolated)
    assert reporter.calls[-1] == ("finish",)
    assert results.run_count("errors") == 1


def test_error_detaching_after_a_test_case_is_an_error_and_the_run_goes_on(
    isolated, tmp_path, monkeypatch
):
    suite_dir = copy_suite(tmp_path, PASSING)
    calls = []

    def detach_once_broken():
        calls.append(len(calls))
        if len(calls) == 1:
            raise RuntimeError("could not detach")

    monkeypatch.setattr(suite.aut, "detach_all", detach_once_broken)
    code, reporter = _run(suite_dir, tags=["~@skip", "~@crash"])
    assert code == 1
    assert calls == [0, 1]
    errors = reporter.steps("error")
    assert [e[2] for e in errors] == ["tst_a: teardown"]
    assert "could not detach" in errors[0][3]
    assert ["OnScenarioEnd", "a passing scenario", False] in _events(isolated)
    assert reporter.calls[-1] == ("finish",)


@pytest.mark.usefixtures("isolated")
def test_unknown_test_case_is_rejected():
    with pytest.raises(ValueError, match="tst_nope"):
        _run(testcases=["tst_nope"])


@pytest.mark.usefixtures("isolated")
def test_failing_test_py_is_an_error(tmp_path):
    suite_dir = copy_suite(tmp_path, UNDEFINED)
    (suite_dir / "tst_b" / "test.py").write_text(
        "source(findFile('scripts', 'no_such_file.py'))\n", encoding="utf-8"
    )
    code, reporter = _run(suite_dir, testcases=["tst_b"])
    assert code == 1
    assert "no_such_file.py" in reporter.steps("error")[0][3]


def test_find_file_and_source(isolated, tmp_path):
    suite_dir = copy_suite(tmp_path, UNDEFINED)
    (suite_dir / "shared" / "scripts" / "extra.py").write_text(
        "EXTRA = 'sourced'\n", encoding="utf-8"
    )
    (suite_dir / "tst_b" / "test.py").write_text(
        "source(findFile('scripts', 'python/bdd.py'))\n"
        "source(findFile('scripts', 'extra.py'))\n"
        "import helpers.EventHelper as h\n"
        "def main():\n"
        "    h.record_event('sourced', EXTRA, findFile('scripts', 'squish.py'))\n",
        encoding="utf-8",
    )
    code, _ = _run(suite_dir, testcases=["tst_b"])
    assert not code
    sourced = next(e for e in _events(isolated) if e[0] == "sourced")
    assert sourced[1] == "sourced"
    assert sourced[2].endswith(os.path.join("shim", "squish.py"))


def test_failing_end_hooks_are_one_scenario(isolated, tmp_path):
    suite_dir = copy_suite(tmp_path, PASSING, end_hooks=True)
    code, reporter = _run(suite_dir, testcases=["tst_b"])
    assert code == 1
    title = "passing feature: OnFeatureEnd"
    assert [c for c in reporter.calls if c[0] == "scenario_start"][-1:] == [
        ("scenario_start", title, [])
    ]
    assert len([c for c in reporter.calls if c[0] == "scenario_start"]) == 2
    errors = reporter.steps("error")
    assert [e[2] for e in errors] == [title, title]
    assert "first end hook broke" in errors[0][3]
    assert "second end hook broke" in errors[1][3]
    assert reporter.calls[-2:] == [("scenario_end", title, True), ("finish",)]
    assert ["OnFeatureEnd", "passing feature"] in _events(isolated)


@pytest.mark.usefixtures("isolated")
def test_failing_test_py_is_a_feature_of_its_own(tmp_path):
    suite_dir = copy_suite(tmp_path, PASSING)
    (suite_dir / "tst_b" / "test.py").write_text("raise KeyError('x')\n", "utf-8")
    _, reporter = _run(suite_dir, testcases=["tst_b"])
    assert reporter.calls[0] == (
        "feature_start",
        "tst_b",
        str((suite_dir / "tst_b" / "test.py").resolve()),
    )
    assert reporter.calls[1] == ("scenario_start", "tst_b/test.py", [])


# --- locations ----------------------------------------------------------------


@pytest.mark.usefixtures("isolated")
def test_scenarios_and_steps_have_their_feature_lines():
    _, reporter = _run(tags=["~@skip"])
    feature = str(SUITE_MIN / "tst_a" / "test.feature")
    located = [(text, loc) for text, loc in reporter.locations if loc]
    assert all(loc.path == feature for _, loc in located)
    lines = [(text, loc.line) for text, loc in located]
    assert lines[:3] == [
        ("tables, doc strings and user data", 12),
        ('Given user "Alice" has been created', 9),  # the Background's step
        ("Given the user has set up the following accounts:", 13),
    ]
    assert str(located[0][1]) == f"{feature}:12"
    # an outline's scenario is its Examples row, its steps are the outline's
    outline = [i for i, (text, _) in enumerate(lines) if text.startswith("syncing")]
    assert [lines[i][1] for i in outline] == [37, 38]
    second = lines[outline[1] :]
    assert second[2:4] == [
        ('When the user waits for folder "two" to be synced', 33),
        ("Then nothing should be remembered", 34),
    ]
    # skipped steps keep theirs
    assert ('Then the remembered value should be "never"', 29) in lines


@pytest.mark.usefixtures("isolated")
def test_hook_steps_and_hook_scenarios_have_no_location(tmp_path):
    suite_dir = copy_suite(tmp_path, PASSING, end_hooks=True)
    _, reporter = _run(suite_dir, testcases=["tst_b"])
    title = "passing feature: OnFeatureEnd"
    assert [loc for text, loc in reporter.locations if text == title] == [None] * 3


# --- several reporters -----------------------------------------------------------


class _Broken(bdd.Reporter):
    def __init__(self, error):
        self.error = error

    def _raise(self, *_args, **_kwargs):
        raise self.error

    feature_start = scenario_start = step = scenario_end = _raise
    attach = interrupted = finish = _raise


def test_reporters_forward_every_call_to_each():
    first, second = RecordingReporter(), RecordingReporter()
    both = bdd.Reporters([first, second])
    location = bdd.Location("f.feature", 3)
    both.feature_start("feature", "f.feature")
    both.scenario_start("scenario", ["@a"], location)
    both.step(bdd.PASSED, "Given x", "", 0.1, location=location)
    both.attach("log.txt")
    both.scenario_end(results.Results("scenario"))
    both.interrupted("interrupted: SIGTERM")
    both.finish()
    assert first.calls == second.calls
    assert [c[0] for c in first.calls] == [
        "feature_start",
        "scenario_start",
        "step",
        "attach",
        "scenario_end",
        "interrupted",
        "finish",
    ]
    assert first.locations == [("scenario", location), ("Given x", location)]


@pytest.mark.parametrize("error", [RuntimeError("broken"), KeyboardInterrupt()])
def test_a_failing_reporter_does_not_stop_the_others(error, capsys):
    after = RecordingReporter()
    both = bdd.Reporters([_Broken(error), after])
    with pytest.raises(type(error)):
        both.step(bdd.FAILED, "Then y", "no", 0.2)
    with pytest.raises(type(error)):
        both.finish()
    assert after.calls == [("step", bdd.FAILED, "Then y", "no"), ("finish",)]
    err = capsys.readouterr().err
    if isinstance(error, Exception):
        assert "[ERROR] reporter _Broken.step failed: RuntimeError: broken" in err
    else:
        assert not err


def test_reporters_raise_the_first_failure():
    both = bdd.Reporters([_Broken(ValueError("one")), _Broken(KeyError("two"))])
    with pytest.raises(ValueError, match="one"):
        both.finish()


@pytest.mark.parametrize("interruption", [KeyboardInterrupt(), suite.Terminated()])
def test_an_interruption_wins_over_an_earlier_failure(interruption):
    both = bdd.Reporters([_Broken(ValueError("one")), _Broken(interruption)])
    with pytest.raises(type(interruption)):
        both.finish()


# --- backslashes in tables, as Squish passes them -------------------------------

BACKSLASH_FEATURE = r'''Feature: backslashes

    Scenario: a table
        Given the files
            | files             |
            | /Folder1/a\\a.txt |
            |   a\|b            |
            | a\nb              |
            | \\ |
        And the text
            """
            a\\b | \|
            """

    Scenario Outline: an outline
        Given the file "<x>" with
            | name |
            | <x>  |

        Examples:
            | x         |
            | dir\\f.txt |
'''


def _run_backslash_feature(tmp_path):
    feature = tmp_path / "test.feature"
    feature.write_text(BACKSLASH_FEATURE, encoding="utf-8")
    runner = bdd.Runner(TagFilter([]))
    names = runner.builtins()
    seen = []

    @names["Given"]("the files")
    def files(context):
        seen.append(("files", context.table))

    @names["Given"]("the text")
    def text(context):
        seen.append(("text", context.multiLineText))

    @names["Given"]('the file "|any|" with')
    def file_with(context, name):
        seen.append(("file", name, context.table))

    runner.run_feature_file(str(feature))
    return seen


def test_table_cells_keep_backslashes_as_written(tmp_path):
    seen = _run_backslash_feature(tmp_path)
    table = seen[0][1]
    assert table[1][0] == "/Folder1/a\\\\a.txt"  # two backslashes
    assert table[2][0] == "a|b"  # an escaped pipe, leading spaces trimmed
    assert table[3][0] == "a\\nb"  # backslash-n, not a newline
    assert table[4][0] == "\\\\"


def test_doc_strings_are_unchanged(tmp_path):
    seen = _run_backslash_feature(tmp_path)
    assert seen[1] == ("text", ["a\\\\b | \\|"])


def test_examples_cells_keep_backslashes(tmp_path):
    seen = _run_backslash_feature(tmp_path)
    assert seen[2] == ("file", "dir\\\\f.txt", [["name"], ["dir\\\\f.txt"]])


@pytest.mark.parametrize(
    "row, cells",
    [
        # a literal pair of backslashes, then the cell's closing pipe
        (r"|C:\\|next|", [r"C:\\", "next"]),
        (r"|a\|b|", ["a|b"]),
        # a pair, then an escaped pipe
        (r"|a\\\|b|", [r"a\\|b"]),
        (r"|a\nb|", [r"a\nb"]),
    ],
)
def test_backslash_pairs_and_escaped_pipes_in_table_rows(row, cells):
    # pylint: disable-next=protected-access
    assert [cell for cell, _ in bdd._split_table_row(row)] == cells


# --- squishinfo -----------------------------------------------------------------

SQUISHINFO = """\
Feature: squishinfo

    Scenario: squishinfo is recorded
        Given squishinfo is recorded
"""

SQUISHINFO_STEP = """\
import squishinfo
from helpers.EventHelper import record_event


@Given('squishinfo is recorded')
def step(context):
    record_event('squishinfo', squishinfo.testCase, squishinfo.resultDir)
"""


def _squishinfo_suite(tmp_path):
    """Test cases tst_b and tst_c that record squishinfo in a step."""
    suite_dir = copy_suite(tmp_path, SQUISHINFO)
    (suite_dir / "shared" / "steps" / "info.py").write_text(SQUISHINFO_STEP, "utf-8")
    shutil.copytree(suite_dir / "tst_b", suite_dir / "tst_c")
    return suite_dir


def test_squishinfo_names_each_test_case_and_the_result_dir(isolated, tmp_path):
    suite_dir = _squishinfo_suite(tmp_path)
    result_dir = str(tmp_path / "results")
    reporter = RecordingReporter()
    code = suite.run_suite(
        str(suite_dir), ["tst_b", "tst_c"], reporter=reporter, result_dir=result_dir
    )
    assert not code
    assert [e for e in _events(isolated) if e[0] == "squishinfo"] == [
        ["squishinfo", str(suite_dir.resolve() / name), result_dir]
        for name in ("tst_b", "tst_c")
    ]


def test_relative_result_dir_is_absolute_in_squishinfo(isolated, tmp_path):
    suite_dir = _squishinfo_suite(tmp_path)
    os.chdir(tmp_path)  # isolated changes back
    code = suite.run_suite(
        str(suite_dir), ["tst_b"], reporter=RecordingReporter(), result_dir="out/html"
    )
    assert not code
    assert [e[2] for e in _events(isolated) if e[0] == "squishinfo"] == [
        str(tmp_path / "out" / "html")
    ]


def test_relative_html_report_dir_is_absolute_in_squishinfo(isolated, tmp_path):
    suite_dir = _squishinfo_suite(tmp_path)
    os.chdir(tmp_path)  # isolated changes back
    args = ["--testsuite", str(suite_dir), "--testcase", "tst_b"]
    assert not cli.main([*args, "--reportgen", "html,test/gui/guiReportUpload"])
    report_dir = tmp_path / "test" / "gui" / "guiReportUpload"
    assert [e[2] for e in _events(isolated) if e[0] == "squishinfo"] == [
        str(report_dir)
    ]
    assert (report_dir / "index.html").is_file()


def test_squishinfo_result_dir_is_empty_without_a_report(isolated, tmp_path):
    suite_dir = _squishinfo_suite(tmp_path)
    code, _ = _run(suite_dir, testcases=["tst_b"])
    assert not code
    assert [e for e in _events(isolated) if e[0] == "squishinfo"] == [
        ["squishinfo", str(suite_dir.resolve() / "tst_b"), ""]
    ]


# --- releasing handles ----------------------------------------------------------


class _Exited:  # pylint: disable=too-few-public-methods
    """A process that has exited, for an application context without one."""

    returncode = 0

    @staticmethod
    def poll():
        return 0


def _fake_context(monkeypatch, name):
    probe = FakeProbe()
    ctx = aut.ApplicationContext(name, _Exited(), probe.client, probe.pid, "")
    monkeypatch.setattr(aut, "_remove", lambda path: None)
    aut._CONTEXTS.append(ctx)  # pylint: disable=protected-access
    return probe


def test_handles_are_released_after_each_scenario(isolated, monkeypatch):
    running = _fake_context(monkeypatch, "running")
    exited = _fake_context(monkeypatch, "exited")
    exited.exit()  # a closed connection is no error

    def record(method, _):
        record_event = importlib.import_module("helpers.EventHelper").record_event
        record_event("request", method)

    running.before_request = record
    try:
        code, reporter = _run(tags=["~@skip"])
    finally:
        running.close()
    assert code == 1  # the crash, nothing else
    assert not [s for s in reporter.steps("error") if "crash" not in s[3]]
    events = [e[:2] for e in _events(isolated)]
    ends = [i for i, e in enumerate(events) if e[0] == "OnScenarioEnd:2"]
    releases = [i for i, e in enumerate(events) if e == ["request", "release"]]
    assert len(releases) == len(ends) == 4
    # each right after the scenario's last OnScenarioEnd hook
    assert releases == [i + 1 for i in ends]
    assert not aut.applicationContextList()  # detached after the test case


class _Unconnected:
    """A client whose socket is no longer connected."""

    @staticmethod
    def request(method, **_):
        raise OSError(errno.ENOTCONN, f"not connected, sending {method!r}")

    @staticmethod
    def close():
        pass


@pytest.mark.parametrize("cause", ["probe error", "socket error"])
@pytest.mark.usefixtures("isolated")
def test_release_that_fails_leaves_the_scenario_passed(monkeypatch, capsys, cause):
    probe = _fake_context(monkeypatch, "broken")
    if cause == "socket error":
        aut._CONTEXTS[-1].client = _Unconnected()  # pylint: disable=protected-access

    def refuse(method, _):
        if method == "release":
            raise FakeError("Internal", "cannot release")

    probe.before_request = refuse
    try:
        code, reporter = _run(tags=["~@skip", "@smokeTest"])
    finally:
        probe.close()
    assert not code
    assert not reporter.steps("error")
    assert ("scenario_end", "tables, doc strings and user data", False) in (
        reporter.calls
    )
    expected = "cannot release" if cause == "probe error" else "not connected"
    err = capsys.readouterr().err
    assert "[WARNING] 'broken' did not release its handles" in err
    assert expected in err


@pytest.mark.usefixtures("isolated")
def test_failing_after_scenario_is_an_error_of_the_scenario(monkeypatch):
    def broken():
        raise ValueError("cannot release")

    monkeypatch.setattr(aut, "release_all", broken)
    code, reporter = _run(tags=["~@skip", "@smokeTest"])
    assert code == 1
    errors = reporter.steps("error")
    assert [e[2] for e in errors] == ["release handles"]
    assert "cannot release" in errors[0][3]
    assert ("scenario_end", "tables, doc strings and user data", True) in (
        reporter.calls
    )


# --- application logs -----------------------------------------------------------

LAUNCHING_FEATURE_HOOKS = """

@OnFeatureStart
def hook(context):
    squish.startApplication('in-feature-start')


@OnFeatureEnd
def hook(context):
    squish.startApplication('in-feature-end')
"""

TWO_SCENARIOS = (
    "Feature: logs\n\n"
    "    Scenario: first\n"
    "        Given the user remembers \"x\"\n\n"
    "    Scenario: second\n"
    '        When the application "in-scenario" is started\n'
)


def _fake_launches(monkeypatch):
    """startApplication only adds the log a launch would write."""

    def launch(cmdline):
        aut._NEW_LOGS.append(f"/logs/{cmdline}.log")  # pylint: disable=protected-access

    monkeypatch.setattr(aut, "startApplication", launch)


def _attachments(reporter):
    """The attached paths, per scenario started."""
    attached, scenario = {}, None
    for call in reporter.calls:
        if call[0] == "scenario_start":
            scenario = call[1]
            attached[scenario] = []
        elif call[0] == "scenario_end":
            scenario = None
        elif call[0] == "attach":
            attached.setdefault(scenario, []).append(call[1])
    return attached


@pytest.mark.usefixtures("isolated")
def test_a_scenario_links_only_the_logs_of_its_own_launches(tmp_path, monkeypatch):
    _fake_launches(monkeypatch)
    suite_dir = copy_suite(tmp_path, TWO_SCENARIOS)
    hooks = suite_dir / "shared" / "scripts" / "bdd_hooks.py"
    hooks.write_text(
        "import squish\n" + hooks.read_text("utf-8") + LAUNCHING_FEATURE_HOOKS,
        "utf-8",
    )
    code, reporter = _run(suite_dir, testcases=["tst_b"])
    assert not code
    assert _attachments(reporter) == {
        "first": [],
        "second": ["/logs/in-scenario.log"],
    }


class _FailingAttach(RecordingReporter):
    def attach(self, path):
        super().attach(path)
        raise OSError(f"cannot link {path}")


@pytest.mark.usefixtures("isolated")
def test_a_failing_attach_still_releases_the_handles(tmp_path, monkeypatch):
    _fake_launches(monkeypatch)
    probe = _fake_context(monkeypatch, "running")
    requests = []
    probe.before_request = lambda method, _: requests.append(method)
    suite_dir = copy_suite(tmp_path, TWO_SCENARIOS)
    reporter = _FailingAttach()
    try:
        code = suite.run_suite(
            str(suite_dir), ["tst_b"], TagFilter([]), False, reporter
        )
    finally:
        probe.close()
    assert code == 1
    assert requests.count("release") == 2  # after each scenario
    errors = reporter.steps("error")
    assert [e[2] for e in errors] == ["release handles"]
    assert "cannot link /logs/in-scenario.log" in errors[0][3]
