// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#include "keys.h"

#include "registry.h"

#include <QHash>
#include <QStringList>

namespace QtGuiProbe {

namespace {

    struct NamedKey
    {
        Qt::Key key;
        QString text;
    };

    // The named keys of Squish's key syntax, by lower-case name.
    const QHash<QString, NamedKey> &namedKeys()
    {
        static const QHash<QString, NamedKey> keys = [] {
            QHash<QString, NamedKey> result{
                {QStringLiteral("return"), {Qt::Key_Return, QStringLiteral("\r")}},
                {QStringLiteral("enter"), {Qt::Key_Enter, QStringLiteral("\r")}},
                {QStringLiteral("tab"), {Qt::Key_Tab, QStringLiteral("\t")}},
                {QStringLiteral("backtab"), {Qt::Key_Backtab, QString()}},
                {QStringLiteral("backspace"), {Qt::Key_Backspace, QStringLiteral("\b")}},
                {QStringLiteral("delete"), {Qt::Key_Delete, QStringLiteral("\x7f")}},
                {QStringLiteral("del"), {Qt::Key_Delete, QStringLiteral("\x7f")}},
                {QStringLiteral("escape"), {Qt::Key_Escape, QStringLiteral("\x1b")}},
                {QStringLiteral("esc"), {Qt::Key_Escape, QStringLiteral("\x1b")}},
                {QStringLiteral("space"), {Qt::Key_Space, QStringLiteral(" ")}},
                {QStringLiteral("up"), {Qt::Key_Up, QString()}},
                {QStringLiteral("down"), {Qt::Key_Down, QString()}},
                {QStringLiteral("left"), {Qt::Key_Left, QString()}},
                {QStringLiteral("right"), {Qt::Key_Right, QString()}},
                {QStringLiteral("home"), {Qt::Key_Home, QString()}},
                {QStringLiteral("end"), {Qt::Key_End, QString()}},
                {QStringLiteral("pageup"), {Qt::Key_PageUp, QString()}},
                {QStringLiteral("pagedown"), {Qt::Key_PageDown, QString()}},
                {QStringLiteral("insert"), {Qt::Key_Insert, QString()}},
            };
            for (int i = 1; i <= 12; ++i) {
                result.insert(QStringLiteral("f%1").arg(i), {Qt::Key(Qt::Key_F1 + i - 1), QString()});
            }
            return result;
        }();
        return keys;
    }

    ProbeError badKeys(const QString &message)
    {
        return {QStringLiteral("BadRequest"), message};
    }

    // The key Qt reports for a typed character: Latin letters and other cased characters by their upper case, as a
    // keyboard does (Key_A for "a", Key_Adiaeresis for "ä"); characters beyond the key codes as Key_unknown.
    Qt::Key keyForCharacter(char32_t codePoint)
    {
        switch (codePoint) {
        case U'\t':
            return Qt::Key_Tab;
        case U'\n':
        case U'\r':
            return Qt::Key_Return;
        default:
            break;
        }
        const char32_t upper = QChar::toUpper(codePoint);
        return upper < 0x01000000 ? Qt::Key(upper) : Qt::Key_unknown;
    }

    KeyStroke characterStroke(char32_t codePoint)
    {
        QString text = QString::fromUcs4(&codePoint, 1);
        if (codePoint == U'\n') {
            text = QStringLiteral("\r");
        }
        return {keyForCharacter(codePoint), Qt::NoModifier, text};
    }

    // A token between '<' and '>': [modifier+]...key. "<+>" and "<Ctrl++>" name the plus key.
    KeyStroke tokenStroke(const QString &token)
    {
        QStringList modifierNames;
        QString keyName;
        if (token == QLatin1String("+")) {
            keyName = token;
        } else if (token.endsWith(QLatin1String("++"))) {
            keyName = QStringLiteral("+");
            modifierNames = token.chopped(2).split(QLatin1Char('+'));
        } else {
            modifierNames = token.split(QLatin1Char('+'));
            keyName = modifierNames.takeLast().trimmed();
        }
        Qt::KeyboardModifiers mods;
        for (const QString &modifier : std::as_const(modifierNames)) {
            const QString name = modifier.trimmed().toLower();
            if (name == QLatin1String("ctrl") || name == QLatin1String("control")) {
                mods |= Qt::ControlModifier;
            } else if (name == QLatin1String("shift")) {
                mods |= Qt::ShiftModifier;
            } else if (name == QLatin1String("alt")) {
                mods |= Qt::AltModifier;
            } else if (name == QLatin1String("meta")) {
                mods |= Qt::MetaModifier;
            } else {
                throw badKeys(QStringLiteral("unknown modifier '%1' in <%2>").arg(modifier, token));
            }
        }
        if (keyName.isEmpty()) {
            throw badKeys(QStringLiteral("no key in <%1>").arg(token));
        }
        KeyStroke stroke;
        const auto named = namedKeys().constFind(keyName.toLower());
        if (named != namedKeys().cend()) {
            stroke = {named->key, mods, named->text};
        } else {
            const QList<uint> codePoints = keyName.toUcs4();
            if (codePoints.size() != 1) {
                throw badKeys(QStringLiteral("unknown key <%1>").arg(token));
            }
            stroke = characterStroke(codePoints.first());
            stroke.mods = mods;
        }
        // Qt reports Shift+Tab as Backtab
        if (stroke.key == Qt::Key_Tab && (mods & Qt::ShiftModifier)) {
            stroke.key = Qt::Key_Backtab;
            stroke.text.clear();
        }
        // with Ctrl, Alt or Meta a key types nothing; with Shift a character types its upper case
        if (mods & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
            stroke.text.clear();
        } else if (mods & Qt::ShiftModifier) {
            stroke.text = stroke.text.toUpper();
        }
        return stroke;
    }

} // namespace

std::vector<KeyStroke> parseSquishKeys(const QString &keys)
{
    std::vector<KeyStroke> strokes;
    const QList<uint> codePoints = keys.toUcs4();
    for (qsizetype i = 0; i < codePoints.size(); ++i) {
        const char32_t codePoint = codePoints[i];
        if (codePoint != U'<') {
            strokes.push_back(characterStroke(codePoint));
            continue;
        }
        if (i + 1 < codePoints.size() && codePoints[i + 1] == U'<') {
            strokes.push_back(characterStroke(U'<'));
            ++i;
            continue;
        }
        qsizetype end = i + 1;
        while (end < codePoints.size() && codePoints[end] != U'>') {
            ++end;
        }
        if (end == codePoints.size()) {
            throw badKeys(QStringLiteral("unterminated key token in '%1'").arg(keys));
        }
        const QString token = QString::fromUcs4(reinterpret_cast<const char32_t *>(codePoints.constData()) + i + 1, end - i - 1);
        if (token.isEmpty()) {
            throw badKeys(QStringLiteral("empty key token in '%1'").arg(keys));
        }
        strokes.push_back(tokenStroke(token));
        i = end;
    }
    return strokes;
}

} // namespace QtGuiProbe
