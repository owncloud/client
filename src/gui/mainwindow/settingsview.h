/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QMap>
#include <QWidget>
#include <QPointer>

namespace OCC {
class IgnoreListEditor;
class SyncLogDialog;

namespace Ui {
    class SettingsView;
}

/**
 * @brief The GeneralSettings class
 * @ingroup gui
 */
class SettingsView : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsView(QWidget *parent = nullptr);
    ~SettingsView() override;

Q_SIGNALS:
    void moveToTrashChanged(bool trashIt);

private Q_SLOTS:
    void saveMiscSettings();
    void loadMiscSettings();
    void slotToggleLaunchOnStartup(bool);
    void slotToggleOptionalDesktopNotifications(bool);
    void slotIgnoreFilesEditor();
    void slotShowLogSettings();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void reloadConfig();
    void loadLanguageNamesIntoDropdown();

    Ui::SettingsView *_ui;
    QPointer<IgnoreListEditor> _ignoreEditor;
    bool _currentlyLoading;
};


} // namespace OCC
