/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "elidedlabel.h"

#include <QResizeEvent>

namespace OCC {

ElidedLabel::ElidedLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent)
    , _text(text)
    , _elideMode(Qt::ElideNone)
{
}

void ElidedLabel::setText(const QString &text)
{
    _text = text;
    QLabel::setText(text);
    update();
}

void ElidedLabel::setElideMode(Qt::TextElideMode elideMode)
{
    _elideMode = elideMode;
    update();
}

void ElidedLabel::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);

    QFontMetrics fm = fontMetrics();
    QString elided = fm.elidedText(_text, _elideMode, event->size().width());
    QLabel::setText(elided);
}
}
