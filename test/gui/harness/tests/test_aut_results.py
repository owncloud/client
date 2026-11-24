# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
# pylint: disable=redefined-outer-name

import builtins
import json
import os
import stat
import sys
import test  # the shim's, first on sys.path
import time
from pathlib import Path

import pytest

import squish
import squishinfo
from harness import aut, builtins_setup, results
from harness import client as client_module

FAKE_AUT = Path(__file__).resolve().parent / "fake_aut.py"


def _script(directory, name, body):
    path = directory / name
    path.write_text(f"#!/bin/sh\n{body}\n", encoding="utf-8")
    path.chmod(path.stat().st_mode | stat.S_IXUSR)
    return path


@pytest.fixture
def auts(tmp_path):
    """An AUT directory with a working, a crashing and a silent application."""
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    _script(bin_dir, "fakeaut", f'exec "{sys.executable}" "{FAKE_AUT}" "$@"')
    _script(bin_dir, "crashaut", "echo 'cannot start: boom' >&2\nexit 3")
    _script(bin_dir, "silentaut", "exec sleep 30")
    argv_file = tmp_path / "argv.json"
    paths = {name: str(bin_dir) for name in ("fakeaut", "crashaut", "silentaut")}
    env = {"FAKE_AUT_ARGV": str(argv_file), "FAKE_AUT_DELAY": "0.3"}
    aut.configure(paths, env)
    yield bin_dir, argv_file, paths, env
    aut.detach_all()


@pytest.fixture
def fresh_results():
    results.reset()
    yield
    results.reset()


@pytest.fixture
def throw_on_failure(monkeypatch):
    monkeypatch.setattr(squish.testSettings, "throwOnFailure", True)


def test_start_application_resolves_passes_args_and_connects(auts):
    bin_dir, argv_file, _, _ = auts
    ctx = squish.startApplication("fakeaut -s --logfile x")
    recorded = json.loads(argv_file.read_text(encoding="utf-8"))
    assert recorded["argv"] == ["-s", "--logfile", "x"]
    assert Path(recorded["cwd"]).resolve() == bin_dir.resolve()
    assert ctx.name == "fakeaut"
    assert ctx.isRunning
    assert ctx.pid == ctx.client.request("hello")["pid"]
    assert client_module.current_client() is ctx.client
    assert squish.currentApplicationContext() is ctx


@pytest.mark.usefixtures("auts")
def test_probe_socket_is_an_absolute_path_in_the_temp_dir(monkeypatch):
    seen = {}
    real_popen = aut.subprocess.Popen

    def spy(*args, **kwargs):
        seen["path"] = kwargs["env"]["QT_GUI_TEST_PROBE"]
        return real_popen(*args, **kwargs)

    monkeypatch.setattr(aut.subprocess, "Popen", spy)
    squish.startApplication("fakeaut")
    path = seen["path"]
    assert os.path.isabs(path)
    assert Path(path).name.startswith(f"qtguiprobe-{os.getpid()}-")


@pytest.mark.usefixtures("auts")
def test_application_that_exits_raises_with_exit_code_and_stderr():
    start = time.monotonic()
    with pytest.raises(RuntimeError, match="exit code 3") as err:
        squish.startApplication("crashaut")
    assert "cannot start: boom" in str(err.value)
    assert time.monotonic() - start < 5
    assert not squish.applicationContextList()


def test_application_without_probe_times_out_and_is_terminated(auts):
    _, _, paths, env = auts
    aut.configure(paths, env, start_timeout_ms=500)
    start = time.monotonic()
    with pytest.raises(RuntimeError, match="500 ms"):
        squish.startApplication("silentaut")
    assert time.monotonic() - start < 5
    assert not squish.applicationContextList()


@pytest.mark.usefixtures("auts")
def test_unknown_application_is_rejected():
    with pytest.raises(RuntimeError, match="nosuchaut"):
        squish.startApplication("nosuchaut --flag")


@pytest.mark.usefixtures("auts")
def test_context_list_and_detach():
    ctx = squish.startApplication("fakeaut")
    assert squish.applicationContextList() == [ctx]
    pid = ctx.pid
    ctx.detach()
    assert not ctx.isRunning
    assert not squish.applicationContextList()
    assert squish.currentApplicationContext() is None
    with pytest.raises(RuntimeError):
        client_module.current_client()
    # Reaped, so the suite's psutil-based wait sees the process gone.
    with pytest.raises(ChildProcessError):
        os.waitpid(pid, os.WNOHANG)


@pytest.mark.usefixtures("fresh_results")
def test_compare_failure_counts_without_throwing():
    assert test.compare(1, 2, "numbers") is False
    assert test.compare(2, 2) is True
    assert test.resultCount("fails") == 1
    assert test.resultCount("passes") == 1


@pytest.mark.usefixtures("fresh_results", "throw_on_failure")
def test_compare_failure_throws_and_counts_with_throw_on_failure():
    with pytest.raises(AssertionError, match="numbers"):
        test.compare(1, 2, "numbers")
    assert test.resultCount("fails") == 1


@pytest.mark.usefixtures("fresh_results", "throw_on_failure")
def test_verify_and_fail_record_and_throw():
    assert test.verify(True, "ok") is True
    with pytest.raises(AssertionError):
        test.verify(False, "not ok")
    with pytest.raises(AssertionError, match="gave up"):
        test.fail("gave up", "details")
    assert test.resultCount("fails") == 2
    assert test.resultCount("passes") == 1


@pytest.mark.usefixtures("fresh_results")
def test_result_count_is_cumulative_across_scenarios():
    results.begin_scenario("first")
    test.fail("one")
    test.warning("careful")
    second = results.begin_scenario("second")
    test.fail("two")
    results.record("errors", "crashed")
    assert test.resultCount("fails") == 2
    assert test.resultCount("errors") == 1
    assert test.resultCount("warnings") == 1
    assert second.count("fails") == 1
    assert second.failed
    assert [e.message for e in second.entries] == ["two", "crashed"]
    assert test.Results is results.Results


@pytest.mark.usefixtures("fresh_results")
def test_result_count_rejects_unknown_kinds():
    with pytest.raises(ValueError):
        test.resultCount("logs")


def test_log_and_video_capture_only_log(capsys):
    test.log("hello")
    test.startVideoCapture()
    test.stopVideoCapture()
    out = capsys.readouterr().out
    assert "hello" in out
    assert "video" in out.lower()


def test_settings_defaults_and_squishinfo():
    assert squish.testSettings.throwOnFailure is False
    assert squish.testSettings.waitForObjectTimeout == 20000
    assert (squishinfo.testCase, squishinfo.resultDir) == ("", "")


def test_install_puts_squish_globals_into_builtins(monkeypatch):
    for name in ("testSettings", "QApplication", "test", "squish", "squishinfo"):
        monkeypatch.delattr(builtins, name, raising=False)
    builtins_setup.install()
    # pylint: disable=no-member
    assert builtins.testSettings is squish.testSettings
    assert builtins.QApplication is squish.QApplication
    assert builtins.test is test
    assert builtins.squishinfo is squishinfo
    # Squish's ``object`` module must never shadow the builtin.
    assert builtins.object is not sys.modules.get("object")
