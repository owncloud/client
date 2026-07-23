/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "folderman.h"
#include "gui/owncloudguilib.h"
#include "platform.h"
#include "traymenucontroller.h"

#include <QMainWindow>
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
class ModalWrapperWidget;


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

    TrayMenuController *tray() const;

    // this is needed primarily to parent message boxes and other temporary views
    // we return QMainWindow to protect access to public functions of the MainWindow that should only be used by true dependents!
    // ie, if you need public functions of MainWindow, it should be injected as its concrete type
    QMainWindow *mainWindow() const;

    // redirect to MainWindow::ensureVisible -> protect access to main window interface
    void ensureVisible() const;

    // hopefully temporary - this is only needed in the updater mess which has no reasonable structure I can currently use to pass the MainWindow
    // again, redirect to MainWindow::showModalWidget to protect access to the rest of the main window interface
    void showModalWidget(ModalWrapperWidget *wrapper) const;

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

    // this is currently fairly empty, but will be moving other manager init stuff in here.
    void setupManagers();

private:
    explicit Application(Platform *platform, const QString &displayLanguage, bool debugMode);

    MainWindow *_mainWin = nullptr;
    MainWindowController *_mainController = nullptr;
    AccountsGuiController *_accountsGuiController = nullptr;
    TrayMenuController *_trayController = nullptr;

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
