/*
 * Copyright (C) by Duncan Mac-Vicar P. <duncan@kde.org>
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

#include "gui/owncloudguilib.h"

#include "folderman.h"
#include "mainwindow/mainwindow.h"
#include "owncloudgui.h"
#include "platform.h"

#include <QPointer>

class QMessageBox;
class QSystemTrayIcon;
class QSocket;

namespace CrashReporter {
class Handler;
}

namespace OCC {

Q_DECLARE_LOGGING_CATEGORY(lcApplication)

class Theme;
class Folder;
class MainWindow;
class MainWindowController;
class AccountsGuiController;

/**
 * @brief The Application class
 * @ingroup gui
 */
class OWNCLOUDGUI_EXPORT Application : public QObject
{
    Q_OBJECT
public:
    static std::unique_ptr<Application> createInstance(Platform *platform, const QString &displayLanguage, bool debugMode);
    ~Application();

    bool debugMode();

    ownCloudGui *gui() const;

    // this is needed primarily to parent message boxes
    MainWindow *mainWindow() { return _mainWin; }

    QString displayLanguage() const;

    /**
     * @brief updateAutoRun will automatically turn on the autorun feature if appropriate
     * @param firstRun indicates whether this is the first time the application is being run.
     *
     * This value is currently gleaned by checking whether the config file exists or not on start, if not, it's a first run.
     * We can't check this inside the function, unfortunately, because the app startup writes the client version to the config
     * "asap", before the Application is instantiated, so we have to capture the state and pass it to this function after the
     * applicaiton exists.
     * Once we have a real application builder this should be a simpler proccess but for now this is what we have.
     */
    void updateAutoRun(bool firstRun);

protected Q_SLOTS:
    void slotUseMonoIconsChanged(bool);
    void slotCleanup();
    void slotAccountStateAdded(AccountState *accountState) const;

private:
    // important! we can't set up the gui's in the ctr - it needs to be a separate step because owncloudgui depends on ocApp to parent
    // it's actions
    void buildAppGuis();

private:
    explicit Application(Platform *platform, const QString &displayLanguage, bool debugMode);

    QPointer<ownCloudGui> _gui = {};

    MainWindow *_mainWin = nullptr;
    MainWindowController *_mainController = nullptr;
    AccountsGuiController *_accountsGuiController = nullptr;

    const bool _debugMode = false;
    QString _displayLanguage;

    static Application *_instance;
    friend Application *ocApp();
};

inline Application *ocApp()
{
    OC_ENFORCE(Application::_instance);
    return Application::_instance;
}

} // namespace OCC
