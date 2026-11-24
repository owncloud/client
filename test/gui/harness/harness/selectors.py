# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Selector value types and their wire encoding."""


class RegularExpression:  # pylint: disable=too-few-public-methods
    def __init__(self, pattern):
        self.pattern = pattern

    def __repr__(self):
        return f"RegularExpression({self.pattern!r})"


class Wildcard:  # pylint: disable=too-few-public-methods
    def __init__(self, pattern):
        self.pattern = pattern

    def __repr__(self):
        return f"Wildcard({self.pattern!r})"


class HandleRef:  # pylint: disable=too-few-public-methods
    """Base of anything that encodes as an object reference (``$handle``)."""

    __slots__ = ()

    @property
    def handle(self):
        raise NotImplementedError


def encode_value(value):
    """Encode one selector value or call argument for the wire."""
    if isinstance(value, RegularExpression):
        return {"$regex": value.pattern}
    if isinstance(value, Wildcard):
        return {"$wildcard": value.pattern}
    if isinstance(value, HandleRef):
        return {"$handle": value.handle}
    if isinstance(value, dict):
        return encode(value)
    if isinstance(value, (list, tuple)):
        return [encode_value(v) for v in value]
    return value


def encode(sel):
    """Encode a selector dict, recursing into nested selectors."""
    return {key: encode_value(value) for key, value in sel.items()}
