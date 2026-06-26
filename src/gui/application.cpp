/*
 * Copyright (C) by Duncan Mac-Vicar P. <duncan@kde.org>
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
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

#include "application.h"


#include "account.h"
#include "accountmanager.h"
#include "accountsgui/accountsguicontroller.h"
#include "accountstate.h"
#include "common/vfs.h"
#include "configfile.h"
#include "folder.h"
#include "folderman.h"
#include "mainwindow/mainwindow.h"
#include "mainwindow/mainwindowcontroller.h"
#include "socketapi/socketapi.h"
#include "theme.h"
#include "traymenucontroller.h"

#ifdef WITH_AUTO_UPDATER
#include "updater/ocupdater.h"
#endif

#if defined(Q_OS_WIN)
#include <qt_windows.h>
#endif

#include <QApplication>
#include <QDesktopServices>
#include <QFileOpenEvent>

namespace OCC {

Q_LOGGING_CATEGORY(lcApplication, "gui.application", QtInfoMsg)

QString Application::displayLanguage() const
{
    return _displayLanguage;
}

TrayMenuController *Application::tray() const
{
    return _trayController;
}

Application *Application::_instance = nullptr;

Application::Application(Platform *platform, const QString &displayLanguage, bool debugMode)
    : _debugMode(debugMode)
    , _displayLanguage(displayLanguage)
{
    platform->migrate();

    qCInfo(lcApplication) << "Plugin search paths:" << qApp->libraryPaths();

    // Check vfs plugins
    if (Theme::instance()->showVirtualFilesOption() && VfsPluginManager::instance().bestAvailableVfsMode() == Vfs::Off) {
        qCWarning(lcApplication) << "Theme wants to show vfs mode, but no vfs plugins are available";
    }
    if (VfsPluginManager::instance().isVfsPluginAvailable(Vfs::WindowsCfApi))
        qCInfo(lcApplication) << "VFS windows plugin is available";

    ConfigFile cfg;

    // this should be called once during application startup to make sure we don't miss any messages
    cfg.configureHttpLogging();

    // The timeout is initialized with an environment variable, if not, override with the value from the config
    if (AbstractNetworkJob::httpTimeout == AbstractNetworkJob::DefaultHttpTimeout) {
        AbstractNetworkJob::httpTimeout = cfg.timeout();
    }

    qApp->setQuitOnLastWindowClosed(false);

    // todo: dc-307 - there should be no setters and hence no notifications from theme
    Theme::instance()->setSystrayUseMonoIcons(cfg.monoIcons());
    connect(Theme::instance(), &Theme::systrayUseMonoIconsChanged, this, &Application::slotUseMonoIconsChanged);

#ifdef WITH_AUTO_UPDATER
    // Update checks
    UpdaterScheduler *updaterScheduler = new UpdaterScheduler(this, this);
    // the updater scheduler takes care of connecting its GUI bits to other components
    (void)updaterScheduler;
#endif

    // Cleanup at Quit.
    connect(qApp, &QCoreApplication::aboutToQuit, this, &Application::slotCleanup);
    qApp->installEventFilter(this);
}

Application::~Application()
{
}

QMainWindow *Application::mainWindow() const
{
    return _mainWin;
}

void Application::ensureVisible() const
{
    if (_mainWin)
        _mainWin->ensureVisible();
}

void Application::showModalWidget(ModalWrapperWidget *wrapper) const
{
    if (_mainWin)
        _mainWin->showModalWidget(wrapper);
}


// move to owncloudgui or wherever we end up consolidating the tray meny/socketApi management
void Application::slotAccountStateAdded(AccountState *accountState) const
{
    if (!accountState || !accountState->account())
        return;
    // Hook up the GUI slots to the account state's Q_SIGNALS:
    Account *account = accountState->account();

    connect(accountState, &AccountState::stateChanged, _trayController, &TrayMenuController::slotComputeOverallSyncStatus);
    connect(account, &Account::serverVersionChanged, _trayController, &TrayMenuController::slotTrayMessageIfServerUnsupported);

    // todo dc-310 - this does not belong here! This can be done in the folder man when it's given a "new" account
    // (eg in load from config or load from new account)
    // Hook up the folder manager slots to the account state's Q_SIGNALS:
    connect(accountState, &AccountState::isConnectedChanged, FolderMan::instance(), &FolderMan::slotIsConnectedChanged);
    connect(account, &Account::serverVersionChanged, FolderMan::instance(), [account] { FolderMan::instance()->slotServerVersionChanged(account); });
}

void Application::slotCleanup()
{
    // by now the credentials are supposed to be persisted
    // don't start async credentials jobs during shutdown
    AccountManager::instance()->save();

    FolderMan::instance()->unloadAndDeleteAllFolders();

    // Remove the account from the account manager so it can be deleted.
    AccountManager::instance()->shutdown();
}

void Application::updateAutoRun(bool firstRun)
{
    if (!firstRun)
        return;

    bool shouldSetAutoStart = firstRun;

    if (Utility::isMac()) {
        // Don't auto start when not being 'installed'
        shouldSetAutoStart = shouldSetAutoStart && QCoreApplication::applicationDirPath().startsWith(QLatin1String("/Applications/"));
    }

    if (shouldSetAutoStart) {
        Utility::setLaunchOnStartup(Theme::instance()->appName(), Theme::instance()->appNameGUI(), true);
    }
}

void Application::slotUseMonoIconsChanged(bool)
{
    _trayController->slotComputeOverallSyncStatus();
}

bool Application::debugMode()
{
    return _debugMode;
}

void Application::buildAppGuis()
{
    Q_ASSERT(!_mainWin && !_trayController);
    _mainWin = new MainWindow();
    _mainController = new MainWindowController(_mainWin, this);

    _accountsGuiController = new AccountsGuiController(AccountManager::instance(), _mainWin, _mainController);
    connect(_mainController, &MainWindowController::requestAccountWizard, _accountsGuiController, &AccountsGuiController::runAccountWizard);

    // Setting up the gui class will allow tray notifications for the
    // setup that follows, like folder setup
    _trayController = new TrayMenuController(this);
    connect(_trayController, &TrayMenuController::requestShowAbout, _mainController, &MainWindowController::onAbout);
    connect(_trayController, &TrayMenuController::requestShowHelp, _mainController, &MainWindowController::onHelp);

    connect(FolderMan::instance()->socketApi(), &SocketApi::shareCommandReceived, _trayController, &TrayMenuController::slotShowShareInBrowser);
}

std::unique_ptr<Application> Application::createInstance(Platform *platform, const QString &displayLanguage, bool debugMode)
{
    Q_ASSERT(!_instance);
    _instance = new Application(platform, displayLanguage, debugMode);
    _instance->buildAppGuis();
    return std::unique_ptr<Application>(_instance);
}

} // namespace OCC
