// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>
#include <Qt>

#include <vector>

namespace QtGuiProbe {

/// One key press and release: the key, the modifiers held, and the text it types (empty for keys that type none).
struct KeyStroke
{
    Qt::Key key;
    Qt::KeyboardModifiers mods;
    QString text;
};

/// Splits Squish key syntax into strokes: every character outside <...> types itself (one stroke per code point),
/// "<<" types "<", and a token names a key, optionally with modifiers joined by '+': <Return>, <Enter>, <Tab>,
/// <Backspace>, <Delete>, <Escape>/<Esc>, <Space>, <Up>, <Down>, <Left>, <Right>, <Home>, <End>, <PageUp>,
/// <PageDown>, <Insert>, <F1>...<F12>, a single character (<A>), and modifiers Ctrl/Control, Shift, Alt, Meta
/// (<Ctrl+A>, <Shift+Tab>, <Alt+F4>). Names are case-insensitive. Throws ProbeError (BadRequest) for an unknown or
/// unterminated token.
std::vector<KeyStroke> parseSquishKeys(const QString &keys);

} // namespace QtGuiProbe
