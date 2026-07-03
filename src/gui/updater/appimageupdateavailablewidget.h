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
#include <QVersionNumber>

namespace Ui {
class AppImageUpdateAvailableWidgetUi;
}

namespace OCC {

/**
 * @brief Dialog shown when updates for the running AppImage are available
 * @ingroup gui
 */
class AppImageUpdateAvailableWidget : public QDialog
{
    Q_OBJECT
public:
    explicit AppImageUpdateAvailableWidget(const QVersionNumber &currentVersion, const QVersionNumber &newVersion, QWidget *parent = nullptr);

    ~AppImageUpdateAvailableWidget() override;

Q_SIGNALS:
    /**
     * Emitted when an update is explicitly skipped by the user.
     */
    void skipUpdateButtonClicked();

private slots:

    void slotSkipUpdate();

private:
    ::Ui::AppImageUpdateAvailableWidgetUi *_ui;
};

}
