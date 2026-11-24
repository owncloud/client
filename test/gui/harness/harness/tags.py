# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Squish's ``--tags`` expressions."""


class TagFilter:  # pylint: disable=too-few-public-methods
    """Selects scenarios by their tags.

    Each expression is one clause and all clauses must hold. Within a clause
    a comma means "or", and a leading ``~`` negates a tag: ``~@skip`` holds
    for every scenario not tagged ``@skip``. No expressions accept everything.
    """

    def __init__(self, exprs):
        self._clauses = [_parse_clause(expr) for expr in exprs]

    def accepts(self, tags):
        """Whether a scenario with these tags (feature tags included) runs."""
        tags = set(tags)
        return all(
            any((tag in tags) != negated for negated, tag in clause)
            for clause in self._clauses
        )


def _parse_clause(expr):
    clause = []
    for term in expr.split(","):
        term = term.strip()
        negated = term.startswith("~")
        tag = term[1:].strip() if negated else term
        if not tag.startswith("@") or len(tag) < 2:
            raise ValueError(
                f"bad tag expression {expr!r}: expected @tag or ~@tag, "
                f"comma-separated, got {term!r}"
            )
        clause.append((negated, tag))
    return clause
