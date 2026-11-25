# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT

import os
import signal
import subprocess
import sys
import xml.etree.ElementTree as ET

import pytest
from suite_copy import SUITE_MIN, copy_suite

from harness import __main__ as cli
from harness import aut, bdd, report, results, suite
from harness.console import ConsoleReporter
from harness.tags import TagFilter

HARNESS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENVVAR = "QT_LOGGING_RULES=sync.httplogger=true;gui.socketapi=false"

SECOND = """\
Feature: second <feature> & co

    Scenario: a <b>bold</b> & "quoted" title
        Given user "Bob" has been created


    Scenario: a failed verification
        Given the user remembers "this"
        Then the remembered value should be "that"
"""

SUITE_MIN_TITLES = [
    "tables, doc strings and user data",
    "the application crashes",
    "syncing after a crash",
    "syncing after a crash",
]


@pytest.fixture
def configured():
    """Leave the applications unconfigured after a CLI run."""
    yield
    aut.configure({}, {})


def _suite(tmp_path, end_hooks=False):
    """suite_min plus tst_b running SECOND."""
    return copy_suite(tmp_path, SECOND, end_hooks)


def _main(*args):
    return cli.main([str(a) for a in args])


def _testcases(junit):
    return ET.parse(junit).getroot().iter("testcase")


@pytest.mark.usefixtures("isolated", "configured")
def test_failing_scenario_exits_with_1_and_reports_every_scenario(tmp_path, capsys):
    junit, html_dir = tmp_path / "out" / "junit.xml", tmp_path / "html"
    code = _main(
        "--testsuite",
        SUITE_MIN,
        "--tags",
        "~@skip",
        "--reportgen",
        f"junitxml,{junit}",
        "--reportgen",
        f"html,{html_dir}",
    )
    assert code == 1
    cases = list(_testcases(junit))
    assert [c.get("name") for c in cases] == SUITE_MIN_TITLES
    assert {c.get("classname") for c in cases} == {"minimal suite"}
    crash = cases[1]
    assert crash.find("failure") is None
    assert "the application under test crashed" in crash.find("error").get("message")
    assert all(c.find("failure") is None and c.find("error") is None for c in cases[2:])
    root = ET.parse(junit).getroot()
    assert (root.get("tests"), root.get("errors"), root.get("failures")) == (
        "4",
        "1",
        "0",
    )
    page = (html_dir / "index.html").read_text("utf-8")
    assert all(title in page for title in SUITE_MIN_TITLES)
    assert "the application under test crashed" in page
    assert "<link" not in page and "<script src" not in page
    assert "the application crashes" in capsys.readouterr().out


@pytest.mark.usefixtures("isolated", "configured")
def test_failure_messages_and_escaping(tmp_path, monkeypatch):
    junit, html_dir = tmp_path / "junit.xml", tmp_path / "html"
    monkeypatch.chdir(tmp_path)
    code = _main(
        "--testsuite",
        _suite(tmp_path),
        "--testcase",
        "tst_b",
        "--reportgen",
        f"html,{html_dir}",
        "--reportgen",
        f"junitxml,{junit}",
    )
    assert code == 1
    cases = {c.get("name"): c for c in _testcases(junit)}
    assert list(cases) == ['a <b>bold</b> & "quoted" title', "a failed verification"]
    failure = cases["a failed verification"].find("failure")
    assert "remembered value" in failure.get("message")
    # relative to the working directory, so that the reports read the same
    # on every machine
    feature = "suite/tst_b/test.feature"
    assert f'Then the remembered value should be "that" ({feature}:9)' in failure.text
    page = (html_dir / "index.html").read_text("utf-8")
    assert "a &lt;b&gt;bold&lt;/b&gt; &amp; &quot;quoted&quot; title" in page
    assert "second &lt;feature&gt; &amp; co" in page
    assert "<b>bold" not in page
    assert "remembered value" in page
    assert f"{feature}:9" in page


@pytest.mark.usefixtures("isolated", "configured")
def test_failing_end_hooks_are_one_testcase(tmp_path):
    junit = tmp_path / "junit.xml"
    suite_dir = _suite(tmp_path, end_hooks=True)
    _main(
        "--testsuite",
        suite_dir,
        "--testcase",
        "tst_b",
        "--reportgen",
        f"junitxml,{junit}",
    )
    hooks = [c for c in _testcases(junit) if c.get("name").endswith("OnFeatureEnd")]
    assert len(hooks) == 1
    text = ET.tostring(hooks[0], encoding="unicode")
    assert "first end hook broke" in text
    assert "second end hook broke" in text
    assert ET.parse(junit).getroot().get("tests") == "3"


@pytest.mark.usefixtures("isolated", "configured")
def test_envvar_and_aut_reach_the_application_configuration(tmp_path, monkeypatch):
    calls = []
    monkeypatch.setattr(
        aut, "configure", lambda *args, **kwargs: calls.append((args, kwargs))
    )
    code = _main(
        "--testsuite",
        SUITE_MIN,
        "--tags",
        "@smokeTest",
        "--envvar",
        ENVVAR,
        "--envvar",
        "EMPTY=",
        "--aut",
        f"fakeaut={tmp_path / 'bin'}",
        "--aut",
        "other=relative/dir",
        "--aut-log-dir",
        "relative/logs",
    )
    assert not code
    assert calls == [
        (
            (
                {
                    "fakeaut": str(tmp_path / "bin"),
                    "other": os.path.abspath("relative/dir"),
                },
                {
                    "QT_LOGGING_RULES": "sync.httplogger=true;gui.socketapi=false",
                    "EMPTY": "",
                },
            ),
            {"log_dir": os.path.abspath("relative/logs")},
        )
    ]


@pytest.mark.usefixtures("isolated", "configured")
def test_without_reports_a_summary_goes_to_stdout(tmp_path, capsys, monkeypatch):
    monkeypatch.chdir(tmp_path)
    code = _main("--testsuite", SUITE_MIN, "--tags", "~@skip")
    assert code == 1
    out = capsys.readouterr().out
    assert "4 scenarios: 3 passed, 0 failed, 1 errors (" in out
    assert "16 steps: 14 passed, 0 failed, 1 errors, 1 skipped" in out
    assert "Feature: minimal suite  (" in out
    assert "the application under test crashed" in out
    assert "::" not in out
    assert [p.name for p in tmp_path.iterdir()] == ["events.jsonl"]


@pytest.mark.usefixtures("isolated", "configured")
def test_quiet_console_prints_nothing_and_writes_the_reports(tmp_path, capsys):
    junit = tmp_path / "junit.xml"
    args = ["--testsuite", SUITE_MIN, "--tags", "~@skip", "--console", "quiet"]
    assert _main(*args, "--reportgen", f"junitxml,{junit}") == 1
    out = capsys.readouterr().out
    assert "Feature:" not in out and "scenarios:" not in out
    assert [c.get("name") for c in _testcases(junit)] == SUITE_MIN_TITLES


@pytest.mark.usefixtures("isolated", "configured")
@pytest.mark.parametrize(
    "console", [[], ["--console", "github"], ["--console", "pretty"]]
)
def test_github_actions_get_the_github_console(tmp_path, capsys, monkeypatch, console):
    summary = tmp_path / "summary.md"
    monkeypatch.setenv("GITHUB_ACTIONS", "true")
    monkeypatch.setenv("GITHUB_WORKSPACE", str(SUITE_MIN.parent))
    monkeypatch.setenv("GITHUB_STEP_SUMMARY", str(summary))
    _main("--testsuite", SUITE_MIN, "--tags", "~@skip", *console)
    out = capsys.readouterr().out
    github = console != ["--console", "pretty"]
    assert ("::group::✓ tables, doc strings and user data  (" in out) == github
    assert ("::error file=suite_min/tst_a/test.feature,line=28," in out) == github
    assert summary.exists() == github


@pytest.mark.usefixtures("isolated", "configured")
def test_paths_resolve_from_the_working_directory(capsys, monkeypatch):
    monkeypatch.chdir(SUITE_MIN.parent.parent)  # tests/
    _main("--testsuite", SUITE_MIN, "--tags", "~@skip", "--console", "pretty")
    out = capsys.readouterr().out
    assert "Feature: minimal suite  (data/suite_min/tst_a/test.feature)" in out
    assert "(data/suite_min/tst_a/test.feature:28)" in out
    assert os.path.isfile("data/suite_min/tst_a/test.feature")


@pytest.mark.usefixtures("isolated", "configured")
@pytest.mark.parametrize("workspace", [None, "link"])
def test_github_annotations_are_relative_to_the_workspace(
    tmp_path, capsys, monkeypatch, workspace
):
    monkeypatch.setenv("GITHUB_ACTIONS", "true")
    if workspace:  # a workspace reached through a symlink
        (tmp_path / "link").symlink_to(SUITE_MIN.parent, target_is_directory=True)
        monkeypatch.setenv("GITHUB_WORKSPACE", str(tmp_path / "link"))
        monkeypatch.chdir(tmp_path)
    else:  # without one, the working directory
        monkeypatch.chdir(SUITE_MIN.parent)
    _main("--testsuite", SUITE_MIN, "--tags", "~@skip")
    out = capsys.readouterr().out
    assert "::error file=suite_min/tst_a/test.feature,line=28," in out


def test_reports_print_nothing(tmp_path, capsys):
    rep = report.Report(junit_paths=[str(tmp_path / "junit.xml")])
    rep.feature_start("feature", "/x/test.feature")
    for _ in _scenario(rep, "broken", bdd.FAILED, "it broke"):
        pass
    rep.finish()
    assert not capsys.readouterr().out
    results.reset()


@pytest.mark.parametrize(
    "args",
    [
        ["--envvar", "NOEQUALS"],
        ["--aut", "nodir"],
        ["--reportgen", "xml3.4,out"],
        ["--reportgen", "html"],
        ["--tags", "skip"],
        ["--console", "loud"],
    ],
)
def test_bad_arguments_exit_with_2(args, capsys):
    with pytest.raises(SystemExit) as info:
        cli.main(["--testsuite", str(SUITE_MIN), *args])
    assert info.value.code == 2
    assert "error" in capsys.readouterr().err


@pytest.mark.parametrize("args", [[], ["--testsuite", "no/such/suite"]])
def test_testsuite_is_required(args, capsys):
    with pytest.raises(SystemExit) as info:
        cli.main(args)
    assert info.value.code == 2
    assert "--testsuite" in capsys.readouterr().err


@pytest.mark.usefixtures("isolated", "configured")
def test_unknown_test_case_exits_with_2(capsys):
    assert _main("--testsuite", SUITE_MIN, "--testcase", "tst_nope") == 2
    assert "tst_nope" in capsys.readouterr().err


# --- the report on its own ----------------------------------------------------


def _scenario(rep, title, result, message=""):
    record = results.begin_scenario(title)
    rep.scenario_start(title, ["@tag"])
    yield
    if result != bdd.PASSED:
        results.record("fails" if result == bdd.FAILED else "errors", message)
    rep.step(result, f"Then {title}", message, 0.5)
    rep.scenario_end(record)


def test_new_screenshots_are_linked_from_their_scenario(tmp_path, monkeypatch):
    report_dir = tmp_path / "reports"
    shots = report_dir / "screenshots"
    shots.mkdir(parents=True)
    (shots / "old.png").write_bytes(b"old")
    monkeypatch.setenv("GUI_TEST_REPORT_DIR", str(report_dir))
    rep = report.Report(html_dirs=[str(report_dir)])
    rep.feature_start("feature", "/x/test.feature")
    for _ in _scenario(rep, "broken", bdd.FAILED, "it <broke>"):
        (shots / "broken.png").write_bytes(b"png")
        rep.attach(str(tmp_path / "extra log.txt"))
    for _ in _scenario(rep, "fine", bdd.PASSED):
        pass
    rep.finish()
    page = (report_dir / "index.html").read_text("utf-8")
    assert page.count('href="screenshots/broken.png"') == 1
    assert "old.png" not in page
    assert "it &lt;broke&gt;" in page
    assert 'href="../extra%20log.txt"' in page
    broken, fine = page.index("broken.png"), page.index(">fine<")
    assert broken < fine
    results.reset()


def test_control_characters_do_not_break_junit(tmp_path):
    junit = tmp_path / "j.xml"
    rep = report.Report(junit_paths=[str(junit)])
    rep.feature_start("feature", "/x/test.feature")
    for _ in _scenario(rep, "ansi", bdd.ERROR, "\x1b[31mred\x1b[0m\x00"):
        pass
    rep.finish()
    error = ET.parse(junit).getroot().find(".//error")
    assert error.get("message") == "[31mred[0m"
    results.reset()


@pytest.mark.usefixtures("isolated", "configured")
def test_html_report_dir_is_squishinfo_result_dir(tmp_path, monkeypatch):
    seen = []
    run_suite = cli.suite.run_suite

    def recording_run_suite(*args, **kwargs):
        seen.append(kwargs.get("result_dir"))
        return run_suite(*args, **kwargs)

    monkeypatch.setattr(cli.suite, "run_suite", recording_run_suite)
    monkeypatch.chdir(tmp_path)
    junit = tmp_path / "junit.xml"
    args = ["--testsuite", SUITE_MIN, "--testcase", "tst_a", "--tags", "@smokeTest"]
    assert not _main(*args, "--reportgen", f"junitxml,{junit}")
    assert not _main(*args, "--reportgen", "html,first", "--reportgen", "html,second")
    assert seen == ["", str(tmp_path / "first")]


# --- writing reports when the run does not finish -------------------------------


class _InterruptedAfterFirstScenario(report.Report):
    def scenario_end(self, record):
        super().scenario_end(record)
        raise KeyboardInterrupt


@pytest.mark.usefixtures("isolated", "configured")
def test_interrupted_run_still_writes_its_reports(tmp_path, capsys):
    junit, html_dir = tmp_path / "junit.xml", tmp_path / "html"
    rep = _InterruptedAfterFirstScenario(
        html_dirs=[str(html_dir)], junit_paths=[str(junit)]
    )
    reporters = bdd.Reporters([rep, ConsoleReporter(sys.stdout)])
    with pytest.raises(KeyboardInterrupt):
        suite.run_suite(
            str(SUITE_MIN), tag_filter=TagFilter(["~@skip"]), reporter=reporters
        )
    assert [t.get("name") for t in _testcases(junit)] == SUITE_MIN_TITLES[:1]
    assert SUITE_MIN_TITLES[0] in (html_dir / "index.html").read_text("utf-8")
    assert "1 scenarios: 1 passed" in capsys.readouterr().out


INTERRUPTED = """\
Feature: interrupted

    Scenario: before the interruption
        Given user "Bob" has been created

    Scenario: the run stops here
        Given user "Carol" has been created
        When the run is interrupted by <how>
        Then the remembered value should be "never"
"""

INTERRUPTING_STEP = """\
import os
import signal
import time


@When('the run is interrupted by |word|')
def step(context, how):
    if how == 'KeyboardInterrupt':
        raise KeyboardInterrupt
    os.kill(os.getpid(), signal.SIGTERM)
    time.sleep(5)  # the signal arrives during the step
    raise AssertionError('SIGTERM did not stop the step')
"""


def _interrupted_suite(tmp_path, how):
    suite_dir = copy_suite(tmp_path, INTERRUPTED.replace("<how>", how))
    steps = suite_dir / "shared" / "steps" / "interrupt.py"
    steps.write_text(INTERRUPTING_STEP, "utf-8")
    return suite_dir


def _run_interrupted(tmp_path, suite_dir):
    junit, html_dir = tmp_path / "junit.xml", tmp_path / "html"
    args = ["--testsuite", suite_dir, "--testcase", "tst_b"]
    _main(*args, "--reportgen", f"junitxml,{junit}", "--reportgen", f"html,{html_dir}")


def _assert_reports_of_interrupted_run(tmp_path, out, how):
    titles = ["before the interruption", "the run stops here"]
    cases = list(_testcases(tmp_path / "junit.xml"))
    assert [t.get("name") for t in cases] == titles
    assert cases[0].find("error") is None
    error = cases[1].find("error")
    assert error is not None
    assert error.get("message") == f"interrupted: {how}"
    page = (tmp_path / "html" / "index.html").read_text("utf-8")
    assert (
        "<tr class=error><td class=status>error</td><td><span class=title>"
        "the run stops here" in page
    )
    assert "2 scenarios: 1 passed, 0 failed, 1 errors" in out
    lines = out.splitlines()
    result = next(i for i, line in enumerate(lines) if "✗ the run stops here" in line)
    assert lines[result].startswith("✗ the run stops here  (error, ")
    assert lines[result + 1 : result + 3] == [
        "    interrupted",
        f"    interrupted: {how}",
    ]


@pytest.mark.usefixtures("isolated", "configured")
def test_interrupted_scenario_is_an_error_in_every_report(tmp_path, capsys):
    suite_dir = _interrupted_suite(tmp_path, "KeyboardInterrupt")
    with pytest.raises(KeyboardInterrupt):  # the process exits non-zero
        _run_interrupted(tmp_path, suite_dir)
    _assert_reports_of_interrupted_run(
        tmp_path, capsys.readouterr().out, "KeyboardInterrupt"
    )


@pytest.mark.usefixtures("isolated", "configured")
def test_sigterm_writes_the_reports_and_exits_with_143(tmp_path, capsys):
    suite_dir = _interrupted_suite(tmp_path, "SIGTERM")
    before = signal.getsignal(signal.SIGTERM)
    with pytest.raises(SystemExit) as stopped:
        _run_interrupted(tmp_path, suite_dir)
    assert stopped.value.code == 143
    assert signal.getsignal(signal.SIGTERM) is before
    _assert_reports_of_interrupted_run(tmp_path, capsys.readouterr().out, "SIGTERM")


def test_sigterm_ends_the_process_with_143(tmp_path):
    suite_dir = _interrupted_suite(tmp_path, "SIGTERM")
    junit = tmp_path / "junit.xml"
    run = subprocess.run(
        [
            sys.executable,
            "-m",
            "harness",
            "--testsuite",
            str(suite_dir),
            "--testcase",
            "tst_b",
            "--reportgen",
            f"junitxml,{junit}",
        ],
        cwd=HARNESS_DIR,
        env={**os.environ, "SUITE_MIN_EVENTS": str(tmp_path / "events.jsonl")},
        capture_output=True,
        text=True,
        timeout=60,
        check=False,
    )
    assert run.returncode == 143, run.stderr
    assert [t.find("error") is not None for t in _testcases(junit)] == [False, True]


def test_scenario_that_never_ended_is_not_passed():
    rep = report.Report()
    rep.scenario_start("unfinished", [])
    rep.step(bdd.PASSED, "Given a step", "", 0.1)
    scenario = rep.scenarios()[0]
    assert scenario.status == report.ERROR
    assert scenario.problems() == [
        ("unfinished", None, "the scenario did not finish", "")
    ]
    assert rep.totals() == {report.PASSED: 0, report.FAILED: 0, report.ERROR: 1}


def test_failing_writer_does_not_skip_the_others(tmp_path, capsys):
    blocked = tmp_path / "blocked"
    blocked.write_text("a file where a directory should be", "utf-8")
    junit, html_dir = tmp_path / "junit.xml", tmp_path / "html"
    rep = report.Report(
        html_dirs=[str(blocked), str(html_dir)],
        junit_paths=[str(blocked / "junit.xml"), str(junit)],
    )
    rep.feature_start("feature", "/x/test.feature")
    for _ in _scenario(rep, "fine", bdd.PASSED):
        pass
    with pytest.raises(RuntimeError, match="2 reports could not be written"):
        rep.finish()
    assert (html_dir / "index.html").is_file()
    assert [t.get("name") for t in _testcases(junit)] == ["fine"]
    err = capsys.readouterr().err.splitlines()
    assert [line.split(":")[0] for line in err] == [
        f"[ERROR] cannot write report {blocked / name}"
        for name in ("index.html", "junit.xml")
    ]
    results.reset()


def test_report_files_are_replaced_whole(tmp_path, monkeypatch):
    junit, html_dir = tmp_path / "junit.xml", tmp_path / "html"
    html_dir.mkdir()
    (html_dir / "index.html").write_text("previous run", "utf-8")
    junit.write_text("previous run", "utf-8")
    rep = report.Report(html_dirs=[str(html_dir)], junit_paths=[str(junit)])

    def broken(*_):
        raise RuntimeError("cannot render")

    monkeypatch.setattr(rep, "html", broken)
    monkeypatch.setattr(ET.ElementTree, "write", broken)
    with pytest.raises(RuntimeError, match="2 reports could not be written"):
        rep.finish()
    assert (html_dir / "index.html").read_text("utf-8") == "previous run"
    assert junit.read_text("utf-8") == "previous run"
    assert sorted(p.name for p in tmp_path.rglob("*")) == [
        "html",
        "index.html",
        "junit.xml",
    ]


@pytest.mark.parametrize("where", ["write", "replace"])
def test_interrupted_report_write_leaves_no_temporary_file(
    tmp_path, monkeypatch, where
):
    path = tmp_path / "junit.xml"
    path.write_text("previous run", "utf-8")

    def interrupt(*_):
        raise KeyboardInterrupt

    def write(stream):
        stream.write(b"partial")
        if where == "write":
            interrupt()

    if where == "replace":
        monkeypatch.setattr(report.os, "replace", interrupt)
    with pytest.raises(KeyboardInterrupt):
        report._write_file(str(path), write)  # pylint: disable=protected-access
    assert path.read_text("utf-8") == "previous run"
    assert sorted(p.name for p in tmp_path.iterdir()) == ["junit.xml"]


def test_ignored_sigterm_stays_ignored():
    before = signal.signal(signal.SIGTERM, signal.SIG_IGN)
    try:
        # pylint: disable-next=protected-access
        assert suite._raise_on_sigterm() is None
        assert signal.getsignal(signal.SIGTERM) == signal.SIG_IGN
    finally:
        signal.signal(signal.SIGTERM, before)
