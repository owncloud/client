# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT

import builtins
import json
import os

import pytest
from suite_copy import SUITE_MIN, copy_suite

from harness import bdd, results, suite
from harness.tags import TagFilter


class RecordingReporter:
    def __init__(self):
        self.calls = []

    def feature_start(self, title, path):
        self.calls.append(("feature_start", title, path))

    def scenario_start(self, title, tags):
        self.calls.append(("scenario_start", title, tags))

    def step(self, result, text, message, duration):
        assert duration >= 0
        self.calls.append(("step", result, text, message))

    def scenario_end(self, record):
        self.calls.append(("scenario_end", record.title, record.failed))

    def attach(self, path):
        self.calls.append(("attach", path))

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
