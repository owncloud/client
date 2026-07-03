/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QDialog>
#include <QScopedPointer>

namespace Ui {
class Ui_NewVersionAvailableWidget;
}

namespace OCC {

class NewVersionAvailableWidget : public QDialog
{
    Q_OBJECT

public:
    explicit NewVersionAvailableWidget(QWidget *parent, const QString &statusMessage);
    ~NewVersionAvailableWidget();

private Q_SLOTS:
    void skipVersion();

Q_SIGNALS:
    void versionSkipped();

private:
    ::Ui::Ui_NewVersionAvailableWidget *_ui;
};

}
