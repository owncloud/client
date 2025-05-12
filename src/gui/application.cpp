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

#include <random>

#include "account.h"
#include "accountmanager.h"
#include "accountstate.h"
#include "common/version.h"
#include "common/vfs.h"
#include "configfile.h"
#include "folder.h"
#include "folderman.h"
#include "settingsdialog.h"
#include "sharedialog.h"
#include "socketapi/socketapi.h"
#include "theme.h"

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

ownCloudGui *Application::gui() const
{
    return _gui;
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
    if (VfsPluginManager::instance().isVfsPluginAvailable(Vfs::WithSuffix))
        qCInfo(lcApplication) << "VFS suffix plugin is available";

    ConfigFile cfg;

    // this should be called once during application startup to make sure we don't miss any messages
    cfg.configureHttpLogging();

    // The timeout is initialized with an environment variable, if not, override with the value from the config
    if (AbstractNetworkJob::httpTimeout == AbstractNetworkJob::DefaultHttpTimeout) {
        AbstractNetworkJob::httpTimeout = cfg.timeout();
    }

    qApp->setQuitOnLastWindowClosed(false);

    Theme::instance()->setSystrayUseMonoIcons(cfg.monoIcons());
    connect(Theme::instance(), &Theme::systrayUseMonoIconsChanged, this, &Application::slotUseMonoIconsChanged);

    // Setting up the gui class will allow tray notifications for the
    // setup that follows, like folder setup
    _gui = new ownCloudGui(this);

    connect(AccountManager::instance(), &AccountManager::accountAdded, this, &Application::slotAccountStateAdded);
    connect(AccountManager::instance(), &AccountManager::accountRemoved, this, &Application::slotAccountStateRemoved);
    for (const auto &ai : AccountManager::instance()->accounts()) {
        slotAccountStateAdded(ai);
    }

    connect(FolderMan::instance()->socketApi(), &SocketApi::shareCommandReceived, _gui.data(), &ownCloudGui::slotShowShareDialog);

    // Refactoring example: this is oversimplified and really belongs in a dedicated app builder impl but the idea is illustrated:
    // don't handling everything "locally" -> request that the best entity for the job do it. Then make the proper connections between
    // requestor and responsible handler in a clearly defined, central location (eg an app builder, but for now, this will do)
    connect(_gui, &ownCloudGui::requestSetUpSyncFoldersForAccount, FolderMan::instance(), &FolderMan::setUpInitialSyncFolders);

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
    // Make sure all folders are gone, otherwise removing the
    // accounts will remove the associated folders from the settings.
    FolderMan::instance()->unloadAndDeleteAllFolders();
}

void Application::slotAccountStateRemoved() const
{
    // if there is no more account, show the wizard.
    if (_gui && AccountManager::instance()->accounts().isEmpty()) {
        // allow to add a new account if there is non any more. Always think
        // about single account theming!
        gui()->runNewAccountWizard();
    }
}

void Application::slotAccountStateAdded(AccountStatePtr accountState) const
{
    // Hook up the GUI slots to the account state's Q_SIGNALS:
    connect(accountState.data(), &AccountState::stateChanged,
        _gui.data(), &ownCloudGui::slotAccountStateChanged);
    connect(accountState->account().data(), &Account::serverVersionChanged,
        _gui.data(), [account = accountState->account().data(), this] {
            _gui->slotTrayMessageIfServerUnsupported(account);
        });

    // Hook up the folder manager slots to the account state's Q_SIGNALS:
    connect(accountState.data(), &AccountState::isConnectedChanged, FolderMan::instance(), &FolderMan::slotIsConnectedChanged);
    connect(accountState->account().data(), &Account::serverVersionChanged, FolderMan::instance(),
        [account = accountState->account().data()] { FolderMan::instance()->slotServerVersionChanged(account); });
    accountState->checkConnectivity();
}

void Application::slotCleanup()
{
    // unload the ui to make sure we no longer react to signals
    _gui->slotShutdown();
    delete _gui;

    // by now the credentials are supposed to be persisted
    // don't start async credentials jobs during shutdown
    AccountManager::instance()->save(false);

    FolderMan::instance()->unloadAndDeleteAllFolders();

    // Remove the account from the account manager so it can be deleted.
    AccountManager::instance()->shutdown();
}

AccountStatePtr Application::addNewAccount(AccountPtr newAccount)
{
    auto *accountMan = AccountManager::instance();

    // first things first: we need to add the new account
    auto accountStatePtr = accountMan->addAccount(newAccount);

    // if one account is configured: enable autostart
    bool shouldSetAutoStart = (accountMan->accounts().size() == 1);
#ifdef Q_OS_MAC
    // Don't auto start when not being 'installed'
    shouldSetAutoStart = shouldSetAutoStart
        && QCoreApplication::applicationDirPath().startsWith(QLatin1String("/Applications/"));
#endif
    if (shouldSetAutoStart) {
        Utility::setLaunchOnStartup(Theme::instance()->appName(), Theme::instance()->appNameGUI(), true);
    }

    // showing the UI to show the user that the account has been added successfully
    _gui->slotShowSettings();

    return accountStatePtr;
}

void Application::slotUseMonoIconsChanged(bool)
{
    _gui->slotComputeOverallSyncStatus();
}

bool Application::debugMode()
{
    return _debugMode;
}

void Application::openVirtualFile(const QString &filename)
{
    QString virtualFileExt = Theme::instance()->appDotVirtualFileSuffix();
    if (!filename.endsWith(virtualFileExt)) {
        qWarning(lcApplication) << "Can only handle file ending in .owncloud. Unable to open" << filename;
        return;
    }
    QString relativePath;
    auto folder = FolderMan::instance()->folderForPath(filename, &relativePath);
    if (!folder) {
        qWarning(lcApplication) << "Can't find sync folder for" << filename;
        // TODO: show a QMessageBox for errors
        return;
    }
    folder->implicitlyHydrateFile(relativePath);
    QString normalName = filename.left(filename.size() - virtualFileExt.size());
    auto con = QSharedPointer<QMetaObject::Connection>::create();
    *con = connect(folder, &Folder::syncFinished, folder, [folder, con, normalName] {
        folder->disconnect(*con);
        if (QFile::exists(normalName)) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(normalName));
        }
    });
}

bool Application::eventFilter(QObject *obj, QEvent *event)
{
#ifdef Q_OS_MAC
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
        qCDebug(lcApplication) << "QFileOpenEvent" << openEvent->file();
        // virtual file, open it after the Folder were created (if the app is not terminated)
        QString fn = openEvent->file();
        QTimer::singleShot(0, this, [this, fn] { openVirtualFile(fn); });
    }
#endif
    return QObject::eventFilter(obj, event);
}

std::unique_ptr<Application> Application::createInstance(Platform *platform, const QString &displayLanguage, bool debugMode)
{
    Q_ASSERT(!_instance);
    _instance = new Application(platform, displayLanguage, debugMode);
    return std::unique_ptr<Application>(_instance);
}

} // namespace OCC
