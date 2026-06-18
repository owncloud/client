/*
 * Copyright (C) Lisa Reese <lisa.reese@kiteworks.com>
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
    void onAbout();

private:
    void buildMenuActions();
    void createSyncErrorsAction();
    void createActivityAction();

    void onAddAccount();
    void onSettings();

    void onQuit();

    MainWindow *_window = nullptr;
    AccountsGuiController *_accountsController = nullptr;
};
}
