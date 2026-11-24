# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Test results: run-wide counters and the per-scenario record."""

from dataclasses import dataclass, field

# The kinds Squish's ``test.resultCount`` knows.
KINDS = ("passes", "fails", "errors", "warnings")


@dataclass
class Entry:
    kind: str  # one of KINDS, or "log"
    message: str
    detail: str = ""


@dataclass
class Results:
    """What happened in one scenario, in order; reports read it."""

    title: str = ""
    entries: list = field(default_factory=list)

    def count(self, kind):
        return sum(1 for entry in self.entries if entry.kind == kind)

    @property
    def failed(self):
        """Whether anything failed or errored."""
        return any(entry.kind in ("fails", "errors") for entry in self.entries)


_totals = dict.fromkeys(KINDS, 0)
_current = Results()


def record(kind, message, detail=""):
    """Record one result in the current scenario and the run-wide totals."""
    if kind != "log":
        _check(kind)
        _totals[kind] += 1
    entry = Entry(kind, str(message), str(detail))
    _current.entries.append(entry)
    return entry


def result_count(kind):
    """How many results of a kind were recorded since the run started."""
    _check(kind)
    return _totals[kind]


def begin_scenario(title):
    """Start a new per-scenario record; the totals carry on."""
    global _current
    _current = Results(title)
    return _current


def current():
    return _current


def reset():
    """Forget everything, as at the start of a run."""
    global _current
    _totals.update(dict.fromkeys(KINDS, 0))
    _current = Results()


def _check(kind):
    if kind not in KINDS:
        raise ValueError(f"unknown result kind {kind!r}, expected one of {KINDS}")
