# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""The runner's command line, after squishrunner's.

Run a suite with ``python -m harness --testsuite DIR`` and optionally
``--testcase NAME``, ``--tags EXPR``, ``--abortOnFail``, ``--envvar K=V``,
``--aut NAME=DIR``, ``--aut-log-dir DIR``, ``--reportgen html,DIR`` or
``--reportgen junitxml,FILE`` and ``--console MODE``. The exit code is 0 only
if nothing failed or errored.
"""

import argparse
import os
import sys

from harness import aut, bdd, console, report, suite
from harness.tags import TagFilter

REPORT_FORMATS = ("html", "junitxml")
CONSOLE_MODES = ("auto", console.PRETTY, console.GITHUB, "quiet")


def _key_value(what):
    """An argparse type for ``KEY=VALUE``; the value may hold more ``=``."""

    def parse(text):
        key, sep, value = text.partition("=")
        if not sep or not key:
            raise argparse.ArgumentTypeError(f"expected {what}, got {text!r}")
        return key, value

    parse.__name__ = what
    return parse


def _reportgen(text):
    kind, sep, path = text.partition(",")
    if kind not in REPORT_FORMATS or not sep or not path:
        raise argparse.ArgumentTypeError(
            f"expected FORMAT,PATH with FORMAT one of {', '.join(REPORT_FORMATS)}, "
            f"got {text!r}"
        )
    # absolute now: the suite changes into each test case's directory
    return kind, os.path.abspath(path)


def parser():
    result = argparse.ArgumentParser(
        prog="python -m harness",
        description="Run a Squish test suite against applications with the probe.",
    )
    add = result.add_argument
    add("--testsuite", required=True, metavar="DIR", help="the suite directory")
    add(
        "--testcase",
        action="append",
        default=[],
        metavar="NAME",
        help="a test case to run (repeatable); by default all tst_* run",
    )
    add(
        "--tags",
        action="append",
        default=[],
        metavar="EXPR",
        help="run only scenarios matching, e.g. ~@skip or @a,@b (repeatable, ANDed)",
    )
    add(
        "--abortOnFail",
        action="store_true",
        help="stop after the first failed scenario",
    )
    add(
        "--envvar",
        action="append",
        default=[],
        type=_key_value("KEY=VALUE"),
        metavar="KEY=VALUE",
        help="set an environment variable for the applications (repeatable)",
    )
    add(
        "--aut",
        action="append",
        default=[],
        type=_key_value("NAME=DIR"),
        metavar="NAME=DIR",
        help="where the application NAME lives (repeatable)",
    )
    add(
        "--aut-log-dir",
        # absolute now: the suite changes into each test case's directory
        type=os.path.abspath,
        metavar="DIR",
        help="write each application's output to DIR/<test case>/<NN>-<name>.log "
        "instead of the console",
    )
    add(
        "--reportgen",
        action="append",
        default=[],
        type=_reportgen,
        metavar="FORMAT,PATH",
        help="html,DIR writes DIR/index.html; junitxml,FILE writes JUnit XML "
        "(repeatable)",
    )
    add(
        "--console",
        choices=CONSOLE_MODES,
        default="auto",
        help="the run on the console: pretty prints features, scenarios and steps; "
        "github adds groups, annotations and a job summary for GitHub Actions; "
        "quiet prints nothing; auto (default) is github where GITHUB_ACTIONS is "
        "true, else pretty",
    )
    return result


def _base():
    """What reports show paths relative to: the repository in GitHub Actions,
    as annotations need, else the working directory, so that the paths
    resolve from the shell. Resolved, as the suite's paths are."""
    return os.path.realpath(os.environ.get("GITHUB_WORKSPACE") or os.getcwd())


def _console(mode, base):
    """The console reporter for a --console mode, or None for quiet."""
    if mode == "auto":
        github = os.environ.get("GITHUB_ACTIONS") == "true"
        mode = console.GITHUB if github else console.PRETTY
    if mode == "quiet":
        return None
    summary = os.environ.get("GITHUB_STEP_SUMMARY") if mode == console.GITHUB else None
    return console.ConsoleReporter(
        sys.stdout, mode, workspace=base, step_summary=summary
    )


def main(argv=None):
    """Run the suite the arguments describe; returns the exit code."""
    arg_parser = parser()
    args = arg_parser.parse_args(argv)
    if not os.path.isdir(args.testsuite):
        arg_parser.error(f"--testsuite: no such directory: {args.testsuite}")
    try:
        tag_filter = TagFilter(args.tags)
    except ValueError as exc:
        arg_parser.error(str(exc))
    aut.configure(
        {name: os.path.abspath(directory) for name, directory in args.aut},
        dict(args.envvar),
        log_dir=args.aut_log_dir,
    )
    html_dirs = [path for kind, path in args.reportgen if kind == "html"]
    base = _base()
    files = report.Report(
        html_dirs=html_dirs,
        junit_paths=[path for kind, path in args.reportgen if kind == "junitxml"],
        base=base,
    )
    console_reporter = _console(args.console, base)
    reporter = bdd.Reporters([files, console_reporter] if console_reporter else [files])
    try:
        return suite.run_suite(
            args.testsuite,
            args.testcase,
            tag_filter,
            args.abortOnFail,
            reporter,
            result_dir=html_dirs[0] if html_dirs else "",
        )
    except ValueError as exc:  # the suite's configuration, as an unknown test case
        print(f"{arg_parser.prog}: error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
