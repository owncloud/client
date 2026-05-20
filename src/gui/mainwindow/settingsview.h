/*
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
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
    void slotToggleLaunchOnStartup(bool);
    void slotToggleOptionalDesktopNotifications(bool);
    void slotIgnoreFilesEditor();
    void loadMiscSettings();

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
