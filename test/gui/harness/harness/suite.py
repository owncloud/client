# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""A Squish test suite: its configuration, environment and test cases."""

import builtins
import importlib
import inspect
import os
import sys
import time
from pathlib import Path

from harness import aut, bdd, builtins_setup, results

SHIM_DIR = Path(__file__).resolve().parent.parent / "shim"
# What findFile returns for Squish's own BDD script; source() skips it.
VIRTUAL_BDD = str(SHIM_DIR / "python" / "bdd.py")


def read_key_values(path):
    """``KEY=VALUE`` lines, as in suite.conf and envs.txt; ``#`` lines are comments."""
    values = {}
    with open(path, encoding="utf-8") as stream:
        for line in stream:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            key, sep, value = line.partition("=")
            if not sep:
                raise ValueError(f"{path}: expected KEY=VALUE, got {line!r}")
            values[key.strip()] = value.strip()
    return values


def run_suite(
    suite_dir, testcases=None, tag_filter=None, abort_on_fail=False, reporter=None
):
    """Run test cases of a suite (all ``tst_*`` by default).

    Returns the exit code: 0 only if nothing failed or errored.
    """
    runner = bdd.Runner(tag_filter, reporter, abort_on_fail)
    return Suite(suite_dir, runner).run(testcases)


class Suite:
    """One suite run, and the Squish functions its ``test.py`` files call."""

    def __init__(self, suite_dir, runner):
        self.dir = Path(suite_dir).resolve()
        conf = self.dir / "suite.conf"
        self.conf = read_key_values(conf) if conf.is_file() else {}
        self.scripts_dir = self.dir / "shared" / "scripts"
        self.runner = runner
        self.testcase_dir = None

    def testcases(self, names=None):
        """The test cases to run: the given ones, TEST_CASES, or all ``tst_*``."""
        if names:
            if missing := [
                n for n in names if not (self.dir / n / "test.py").is_file()
            ]:
                raise ValueError(
                    f"no such test case in {self.dir}: {', '.join(missing)}"
                )
            return list(names)
        if "TEST_CASES" in self.conf:
            return self.testcases(self.conf["TEST_CASES"].split())
        return sorted(
            p.parent.name for p in self.dir.glob("tst_*/test.py") if p.is_file()
        )

    def run(self, names=None):
        names = self.testcases(names)
        if "ENVVARS" in self.conf:
            os.environ.update(read_key_values(self.dir / self.conf["ENVVARS"]))
        for path in (str(self.scripts_dir), str(SHIM_DIR)):
            while path in sys.path:
                sys.path.remove(path)
            sys.path.insert(0, path)
        builtins_setup.install()
        builtins_setup.install_names(self.builtins())
        results.reset()
        for name in names:
            self._run_testcase(name)
            if self.runner.aborted:
                break
        self.runner.reporter.finish()
        failed = results.run_count("fails") + results.run_count("errors")
        return 1 if failed else 0

    def builtins(self):
        """Squish's BDD entry points, by their Squish names."""
        return {
            **self.runner.builtins(),
            "source": self.source,
            "findFile": self.find_file,
            "setupHooks": self.setup_hooks,
            "collectStepDefinitions": self.collect_step_definitions,
            "runFeatureFile": self.run_feature_file,
        }

    def _run_testcase(self, name):
        """Run one test case as Squish does: a fresh start, in its directory."""
        self.testcase_dir = self.dir / name
        self._forget_suite_modules()
        self.runner.reset_definitions()
        results.begin_test_case()
        cwd = os.getcwd()
        os.chdir(self.testcase_dir)
        start = time.monotonic()
        fails_before = results.run_count("fails")
        try:
            namespace = _namespace(self.testcase_dir / "test.py", "__main__")
            _exec_file(self.testcase_dir / "test.py", namespace)
            if callable(main := namespace.get("main")):
                main()
        except Exception as exc:  # pylint: disable=broad-exception-caught
            title = f"{name}/test.py"
            results.begin_scenario(title)
            result, message = bdd.outcome(exc, fails_before)
            self.runner.reporter.feature_start(name, str(self.testcase_dir / "test.py"))
            self.runner.report_problems(
                title, [(result, message, time.monotonic() - start)]
            )
        finally:
            os.chdir(cwd)
            aut.detach_all()

    def _forget_suite_modules(self):
        """Drop the suite's modules, so each test case imports them afresh."""
        for name, module in list(sys.modules.items()):
            paths = [getattr(module, "__file__", None) or ""]
            paths += list(getattr(module, "__path__", None) or [])
            if any(_is_within(p, self.scripts_dir) for p in paths if p):
                del sys.modules[name]

    def _resolve(self, path):
        return (self.testcase_dir / path).resolve()

    # -- Squish's functions, called from test.py -------------------------------

    def source(self, path):
        """Run a file in the caller's globals; Squish's own bdd.py is built in."""
        if os.path.abspath(path) == VIRTUAL_BDD:
            return
        caller = inspect.currentframe().f_back
        _exec_file(self._resolve(path), caller.f_globals)

    def find_file(self, kind, name):
        """Find a ``scripts`` file in the suite's shared scripts or the shim."""
        if kind != "scripts":
            raise ValueError(f"findFile: unsupported kind {kind!r}, only 'scripts'")
        if name.replace("\\", "/") == "python/bdd.py":
            return VIRTUAL_BDD
        directories = (self.scripts_dir, SHIM_DIR)
        for directory in directories:
            if (candidate := directory / name).is_file():
                return str(candidate)
        searched = ", ".join(str(d) for d in directories)
        raise LookupError(f"findFile: no script {name!r} in {searched}")

    def setup_hooks(self, *paths):
        """Run hook files, relative to the test case directory."""
        for path in paths:
            path = self._resolve(path)
            _exec_file(path, _namespace(path, path.stem))

    def collect_step_definitions(self, *directories):
        """Run the step files of each directory; missing directories are skipped.

        Directories given first take precedence for a pattern defined twice.
        """
        for directory in directories:
            directory = self._resolve(directory)
            if directory.is_dir():
                for path in sorted(directory.glob("*.py")):
                    _exec_file(path, _namespace(path, path.stem))

    def run_feature_file(self, path):
        self.runner.run_feature_file(str(self._resolve(path)))


def _namespace(path, name):
    """Globals for a suite file. Squish's ``object`` lives here, not in
    builtins, where it would replace Python's ``object``."""
    return {
        "__name__": name,
        "__file__": str(path),
        "__builtins__": builtins,
        "object": importlib.import_module("object"),
    }


def _exec_file(path, namespace):
    with open(path, encoding="utf-8") as stream:
        code = compile(stream.read(), str(path), "exec")
    exec(code, namespace)  # pylint: disable=exec-used


def _is_within(path, directory):
    try:
        Path(path).resolve().relative_to(directory)
    except ValueError:
        return False
    return True
