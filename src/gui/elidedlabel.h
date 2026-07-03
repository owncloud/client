/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ELIDEDLABEL_H
#define ELIDEDLABEL_H

#include <QLabel>

namespace OCC {

/// Label that can elide its text
class ElidedLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ElidedLabel(const QString &text, QWidget *parent = nullptr);

    void setText(const QString &text);
    const QString &text() const { return _text; }

    void setElideMode(Qt::TextElideMode elideMode);
    Qt::TextElideMode elideMode() const { return _elideMode; }

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QString _text;
    Qt::TextElideMode _elideMode;
};
}

#endif
