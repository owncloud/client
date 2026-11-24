# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT

import os
import xml.etree.ElementTree as ET

import pytest
from suite_copy import SUITE_MIN, copy_suite

from harness import __main__ as cli
from harness import aut, bdd, report, results

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
def test_failure_messages_and_escaping(tmp_path):
    junit, html_dir = tmp_path / "junit.xml", tmp_path / "html"
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
    assert 'Then the remembered value should be "that"' in failure.text
    page = (html_dir / "index.html").read_text("utf-8")
    assert "a &lt;b&gt;bold&lt;/b&gt; &amp; &quot;quoted&quot; title" in page
    assert "second &lt;feature&gt; &amp; co" in page
    assert "<b>bold" not in page
    assert "remembered value" in page


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
    monkeypatch.setattr(aut, "configure", lambda *args: calls.append(args))
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
    )
    assert not code
    assert calls == [
        (
            {
                "fakeaut": str(tmp_path / "bin"),
                "other": os.path.abspath("relative/dir"),
            },
            {
                "QT_LOGGING_RULES": "sync.httplogger=true;gui.socketapi=false",
                "EMPTY": "",
            },
        )
    ]


@pytest.mark.usefixtures("isolated", "configured")
def test_without_reports_a_summary_goes_to_stdout(tmp_path, capsys, monkeypatch):
    monkeypatch.chdir(tmp_path)
    code = _main("--testsuite", SUITE_MIN, "--tags", "~@skip")
    assert code == 1
    out = capsys.readouterr().out
    assert "4 scenarios: 3 passed, 0 failed, 1 errors" in out
    assert "the application under test crashed" in out
    assert [p.name for p in tmp_path.iterdir()] == ["events.jsonl"]


@pytest.mark.parametrize(
    "args",
    [
        ["--envvar", "NOEQUALS"],
        ["--aut", "nodir"],
        ["--reportgen", "xml3.4,out"],
        ["--reportgen", "html"],
        ["--tags", "skip"],
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
    rep = report.Report(html_dirs=[str(report_dir)], stream=None)
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
    rep = report.Report(junit_paths=[str(junit)], stream=None)
    rep.feature_start("feature", "/x/test.feature")
    for _ in _scenario(rep, "ansi", bdd.ERROR, "\x1b[31mred\x1b[0m\x00"):
        pass
    rep.finish()
    error = ET.parse(junit).getroot().find(".//error")
    assert error.get("message") == "[31mred[0m"
    results.reset()
