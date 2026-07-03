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

namespace Ui {
class UpdateDownloadedWidget;
}

namespace OCC {

class UpdateDownloadedWidget : public QDialog
{
    Q_OBJECT

public:
    explicit UpdateDownloadedWidget(QWidget *parent, const QString &statusMessage);
    ~UpdateDownloadedWidget();

private:
    ::Ui::UpdateDownloadedWidget *_ui;
};

}
