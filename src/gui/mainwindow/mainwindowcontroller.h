/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>

namespace OCC {

class MainWindow;
class AccountsGuiController;

class MainWindowController : public QObject
{
    Q_OBJECT
public:
    explicit MainWindowController(MainWindow *window, QObject *parent = nullptr);

    // public for now
    void setup();
    // called from tray menu, too. make this a signal/slot connection between the tray menu controller and main window controller
    void onAbout();
    void onHelp();

signals:
    void requestAccountWizard();

private:
    void buildMenuActions();
    void createSyncErrorsAction();
    void createActivityAction();

    void onSettings();
    void onQuit();

    MainWindow *_window = nullptr;
};
}
