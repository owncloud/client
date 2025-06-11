/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
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

#include "owncloudgui.h"
#include "aboutdialog.h"
#include "account.h"
#include "accountmanager.h"
#include "accountstate.h"
#include "application.h"
#include "common/restartmanager.h"
#include "common/syncjournalfilerecord.h"
#include "configfile.h"
#include "folderman.h"
#include "folderwizard/folderwizard.h"
#include "gui/accountsettings.h"
#include "gui/commonstrings.h"
#include "gui/networkinformation.h"
#include "guiutility.h"
#include "libsync/theme.h"
#include "logbrowser.h"
#include "openfilemanager.h"
#include "progressdispatcher.h"
#include "settingsdialog.h"
#include "setupwizardcontroller.h"
#include "sharedialog.h"

#include "libsync/graphapi/spacesmanager.h"

#include "resources/resources.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QHBoxLayout>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

using namespace std::chrono_literals;

namespace OCC {

ownCloudGui::ownCloudGui(Application *parent)
    : QObject(parent)
    , _tray(new Systray(this))
    , _settingsDialog(new SettingsDialog(this))
    , _recentActionsMenu(nullptr)
    , _app(parent)
{
    connect(_tray, &QSystemTrayIcon::activated,
        this, &ownCloudGui::slotTrayClicked);

    setupActions();
    setupContextMenu();

    // init systry
    slotComputeOverallSyncStatus();
    _tray->show();

    // Refactoring todo: we have a very nasty habit of connecting to globally available managers, etc inside constructors or similar.
    // this should only happen when we are dealing with a formal dependency: eg a dep passed by injection and kept as member,
    // or a dep that is instantiated inside the class (also with a member kept)
    // in cases like this one, the external deps should be instantiated externally and connected externally. This is normally part
    // of an app building routine. The global singletons have to go and this is an important step to achieving that.

    ProgressDispatcher *pd = ProgressDispatcher::instance();
    connect(pd, &ProgressDispatcher::progressInfo, this,
        &ownCloudGui::slotUpdateProgress);

    FolderMan *folderMan = FolderMan::instance();
    connect(folderMan, &FolderMan::folderSyncStateChange,
        this, &ownCloudGui::slotSyncStateChange);

    connect(AccountManager::instance(), &AccountManager::accountAdded,
        this, &ownCloudGui::updateContextMenuNeeded);
    connect(AccountManager::instance(), &AccountManager::accountRemoved,
        this, &ownCloudGui::updateContextMenuNeeded);
}

ownCloudGui::~ownCloudGui()
{
    delete _settingsDialog;
}

// This should rather be in application.... or rather in ConfigFile?
void ownCloudGui::slotOpenSettingsDialog()
{
    // if account is set up, start the configuration wizard.
    if (!AccountManager::instance()->accounts().isEmpty()) {
        if (QApplication::activeWindow() != _settingsDialog) {
            slotShowSettings();
        } else {
            _settingsDialog->close();
        }
    } else {
        qCInfo(lcApplication) << "No configured folders yet, starting setup wizard";
        runNewAccountWizard();
    }
}

void ownCloudGui::slotTrayClicked(QSystemTrayIcon::ActivationReason reason)
{
    if (_workaroundFakeDoubleClick) {
        static QElapsedTimer last_click;
        if (last_click.isValid() && last_click.elapsed() < 200) {
            return;
        }
        last_click.start();
    }

    // Left click
    if (reason == QSystemTrayIcon::Trigger) {
#ifdef Q_OS_MAC
        // on macOS, a left click always opens menu.
        // However if the settings dialog is already visible but hidden
        // by other applications, this will bring it to the front.
        if (_settingsDialog->isVisible()) {
            raise();
        }
#else
        slotOpenSettingsDialog();
#endif
    }
    // FIXME: Also make sure that any auto updater dialogue https://github.com/owncloud/client/issues/5613
    // or SSL error dialog also comes to front.
}

void ownCloudGui::slotSyncStateChange(Folder *folder)
{
    slotComputeOverallSyncStatus();
    updateContextMenuNeeded();

    if (!folder) {
        return; // Valid, just a general GUI redraw was needed.
    }

    auto result = folder->syncResult();

    qCInfo(lcApplication) << "Sync state changed for folder " << folder->remoteUrl().toString() << ": " << Utility::enumToDisplayName(result.status());
}

void ownCloudGui::slotFoldersChanged()
{
    slotComputeOverallSyncStatus();
    updateContextMenuNeeded();
}

void ownCloudGui::slotOpenPath(const QString &path)
{
    showInFileManager(path);
}

void ownCloudGui::slotAccountStateChanged()
{
    updateContextMenuNeeded();
    slotComputeOverallSyncStatus();
}

void ownCloudGui::slotTrayMessageIfServerUnsupported(Account *account)
{
    if (account->serverSupportLevel() != Account::ServerSupportLevel::Supported) {
        slotShowTrayMessage(tr("Unsupported Server Version"),
            tr("The server on account %1 runs an unsupported version %2. "
               "Using this client with unsupported server versions is untested and "
               "potentially dangerous. Proceed at your own risk.")
                .arg(account->displayNameWithHost(), account->capabilities().status().versionString()));
    }
}

void ownCloudGui::slotComputeOverallSyncStatus()
{
    auto getIcon = [this](const SyncResult &result) { return Theme::instance()->themeTrayIcon(result, contextMenuVisible()); };
    auto getIconFromStatus = [getIcon](const SyncResult::Status &status) { return getIcon(SyncResult{status}); };
    bool allSignedOut = true;
    bool allPaused = true;
    bool allDisconnected = true;
    QVector<AccountStatePtr> problemAccounts;
    auto setStatusText = [&](const QString &text) {
        // Don't overwrite the status if we're currently syncing
        if (FolderMan::instance()->isAnySyncRunning())
            return;
        _actionStatus->setText(text);
    };

    for (const auto &a : AccountManager::instance()->accounts()) {
        if (!a->isSignedOut()) {
            allSignedOut = false;
        }
        if (!a->isConnected()) {
            problemAccounts.append(a);
        } else {
            allDisconnected = false;
        }
    }

    const auto &map = FolderMan::instance()->folders();
    for (auto *f : map) {
        if (!f->syncPaused()) {
            allPaused = false;
        }
    }

    if (!problemAccounts.empty()) {
        _tray->setIcon(getIconFromStatus(SyncResult::Status::Offline));
        if (allDisconnected) {
            setStatusText(tr("Disconnected"));
        } else {
            setStatusText(tr("Disconnected from some accounts"));
        }
#ifdef Q_OS_WIN
        // Windows has a 128-char tray tooltip length limit.
        QStringList accountNames;
        for (const auto &a : std::as_const(problemAccounts)) {
            accountNames.append(a->account()->displayNameWithHost());
        }
        _tray->setToolTip(tr("Disconnected from %1").arg(accountNames.join(QLatin1String(", "))));
#else
        QStringList messages;
        messages.append(tr("Disconnected from accounts:"));
        for (const auto &a : std::as_const(problemAccounts)) {
            QString message = tr("Account %1").arg(a->account()->displayNameWithHost());
            if (!a->connectionErrors().empty()) {
                message += QLatin1String("\n") + a->connectionErrors().join(QLatin1String("\n"));
            }
            messages.append(message);
        }
        _tray->setToolTip(messages.join(QLatin1String("\n\n")));
#endif
        return;
    }

    if (allSignedOut) {
        _tray->setIcon(getIconFromStatus(SyncResult::Status::Offline));
        _tray->setToolTip(tr("Please sign in"));
        setStatusText(tr("Signed out"));
        return;
    } else if (allPaused) {
        _tray->setIcon(getIconFromStatus(SyncResult::Paused));
        _tray->setToolTip(tr("Account synchronization is disabled"));
        setStatusText(tr("Synchronization is paused"));
        return;
    }

    // display the info of the least successful sync (eg. do not just display the result of the latest sync)
    QString trayMessage;

    auto trayOverallStatusResult = FolderMan::trayOverallStatus(map);

    const QIcon statusIcon = getIcon(trayOverallStatusResult.overallStatus());
    _tray->setIcon(statusIcon);

    // create the tray blob message, check if we have an defined state
#ifdef Q_OS_WIN
    // Windows has a 128-char tray tooltip length limit.
    trayMessage = FolderMan::instance()->trayTooltipStatusString(trayOverallStatusResult.overallStatus(), false);
#else
    QStringList allStatusStrings;
    for (auto *folder : map) {
        QString folderMessage = FolderMan::trayTooltipStatusString(
            folder->syncResult(),
            folder->syncPaused());
        allStatusStrings += tr("Folder %1: %2").arg(folder->shortGuiLocalPath(), folderMessage);
    }
    trayMessage = allStatusStrings.join(QLatin1String("\n"));
#endif
    _tray->setToolTip(trayMessage);

    switch (trayOverallStatusResult.overallStatus().status()) {
    case SyncResult::Problem:
        if (trayOverallStatusResult.overallStatus().hasUnresolvedConflicts()) {
            setStatusText(tr("Unresolved %1 conflicts").arg(QString::number(trayOverallStatusResult.overallStatus().numNewConflictItems())));
            break;
        } else if (trayOverallStatusResult.overallStatus().numBlacklistErrors() != 0) {
            setStatusText(tr("Ignored errors %1").arg(QString::number(trayOverallStatusResult.overallStatus().numBlacklistErrors())));
            break;
        }
        [[fallthrough]];
    case SyncResult::Success: {
        QString lastSyncDoneString;
        // display only the time in case the last sync was today
        if (QDateTime::currentDateTime().date() == trayOverallStatusResult.lastSyncDone.date()) {
            lastSyncDoneString = QLocale().toString(trayOverallStatusResult.lastSyncDone.time());
        } else {
            lastSyncDoneString = QLocale().toString(trayOverallStatusResult.lastSyncDone);
        }
        setStatusText(tr("Up to date (%1)").arg(lastSyncDoneString));
    } break;
    case SyncResult::Undefined:
        _tray->setToolTip(tr("There are no sync folders configured."));
        setStatusText(tr("No sync folders configured"));
        break;
    default:
        setStatusText(FolderMan::instance()->trayTooltipStatusString(trayOverallStatusResult.overallStatus(), false));
    }
}

void ownCloudGui::addAccountContextMenu(AccountStatePtr accountState, QMenu *menu)
{
    menu->addAction(CommonStrings::showInWebBrowser(), this, [accountState] { QDesktopServices::openUrl(accountState->account()->url()); });

    FolderMan *folderMan = FolderMan::instance();
    const auto &map = folderMan->folders();
    bool onePaused = false;
    for (auto *folder : map) {
        if (folder->accountState() != accountState.data()) {
            continue;
        }

        if (folder->syncPaused()) {
            onePaused = true;
        }
        menu->addAction(tr("Open folder '%1'").arg(folder->shortGuiLocalPath()), this, [this, folder] { slotFolderOpenAction(folder); });
    }

    menu->addSeparator();
    if (onePaused) {
        menu->addAction(tr("Resume synchronization"), this, [accountState, this] { setPauseOnAllFoldersHelper({accountState}, false); });
    } else {
        menu->addAction(tr("Stop synchronization"), this, [accountState, this] { setPauseOnAllFoldersHelper({accountState}, true); });
    }

    if (accountState->isSignedOut()) {
        menu->addAction(tr("Log in..."), this, [accountState] { accountState->signIn(); });
    } else {
        menu->addAction(tr("Log out"), this, [accountState] { accountState->signOutByUi(); });
    }
}

SettingsDialog *ownCloudGui::settingsDialog() const
{
    return _settingsDialog;
}

void ownCloudGui::slotContextMenuAboutToShow()
{
    _contextMenuVisibleManual = true;

    // Update icon in sys tray, as it might change depending on the context menu state
    slotComputeOverallSyncStatus();

    if (!_workaroundNoAboutToShowUpdate) {
        updateContextMenu();
    }
}

void ownCloudGui::slotContextMenuAboutToHide()
{
    _contextMenuVisibleManual = false;

    // Update icon in sys tray, as it might change depending on the context menu state
    slotComputeOverallSyncStatus();
}

bool ownCloudGui::contextMenuVisible() const
{
    // On some platforms isVisible doesn't work and always returns false,
    // elsewhere aboutToHide is unreliable.
    if (_workaroundManualVisibility)
        return _contextMenuVisibleManual;
    return _contextMenu->isVisible();
}

void ownCloudGui::hideAndShowTray()
{
    _tray->hide();
    _tray->show();
}

static bool updateWhileVisible()
{
    static QByteArray var = qgetenv("OWNCLOUD_TRAY_UPDATE_WHILE_VISIBLE");
    if (var == "1") {
        return true;
    } else if (var == "0") {
        return false;
    } else {
        // triggers bug on OS X: https://bugreports.qt.io/browse/QTBUG-54845
        // or flickering on Xubuntu
        return false;
    }
}

static QByteArray envForceQDBusTrayWorkaround()
{
    static QByteArray var = qgetenv("OWNCLOUD_FORCE_QDBUS_TRAY_WORKAROUND");
    return var;
}

static QByteArray envForceWorkaroundShowAndHideTray()
{
    static QByteArray var = qgetenv("OWNCLOUD_FORCE_TRAY_SHOW_HIDE");
    return var;
}

static QByteArray envForceWorkaroundNoAboutToShowUpdate()
{
    static QByteArray var = qgetenv("OWNCLOUD_FORCE_TRAY_NO_ABOUT_TO_SHOW");
    return var;
}

static QByteArray envForceWorkaroundFakeDoubleClick()
{
    static QByteArray var = qgetenv("OWNCLOUD_FORCE_TRAY_FAKE_DOUBLE_CLICK");
    return var;
}

static QByteArray envForceWorkaroundManualVisibility()
{
    static QByteArray var = qgetenv("OWNCLOUD_FORCE_TRAY_MANUAL_VISIBILITY");
    return var;
}

void ownCloudGui::setupContextMenu()
{
    if (_contextMenu) {
        return;
    }

    _contextMenu.reset(new QMenu());
    _contextMenu->setTitle(Theme::instance()->appNameGUI());

    _recentActionsMenu = new QMenu(tr("Recent Changes"), _contextMenu.data());

    // this must be called only once after creating the context menu, or
    // it will trigger a bug in Ubuntu's SNI bridge patch (11.10, 12.04).
    _tray->setContextMenu(_contextMenu.data());

    auto applyEnvVariable = [](bool *sw, const QByteArray &value) {
        if (value == "1")
            *sw = true;
        if (value == "0")
            *sw = false;
    };

    // This is an old compound flag that people might still depend on
    bool qdbusmenuWorkarounds = false;
    applyEnvVariable(&qdbusmenuWorkarounds, envForceQDBusTrayWorkaround());
    if (qdbusmenuWorkarounds) {
        _workaroundFakeDoubleClick = true;
        _workaroundNoAboutToShowUpdate = true;
        _workaroundShowAndHideTray = true;
    }

#ifdef Q_OS_MAC
    // https://bugreports.qt.io/browse/QTBUG-54633
    _workaroundNoAboutToShowUpdate = true;
    _workaroundManualVisibility = true;
#endif

#ifdef Q_OS_LINUX
    // For KDE sessions if the platform plugin is missing,
    // neither aboutToShow() updates nor the isVisible() call
    // work. At least aboutToHide is reliable.
    // https://github.com/owncloud/client/issues/6545
    static QByteArray xdgCurrentDesktop = qgetenv("XDG_CURRENT_DESKTOP");
    static QByteArray desktopSession = qgetenv("DESKTOP_SESSION");
    bool isKde =
        xdgCurrentDesktop.contains("KDE")
        || desktopSession.contains("plasma")
        || desktopSession.contains("kde");
    QObject *platformMenu = reinterpret_cast<QObject *>(_tray->contextMenu()->platformMenu());
    if (isKde && platformMenu && platformMenu->metaObject()->className() == QByteArrayLiteral("QDBusPlatformMenu")) {
        _workaroundManualVisibility = true;
        _workaroundNoAboutToShowUpdate = true;
    }
#endif

    applyEnvVariable(&_workaroundNoAboutToShowUpdate, envForceWorkaroundNoAboutToShowUpdate());
    applyEnvVariable(&_workaroundFakeDoubleClick, envForceWorkaroundFakeDoubleClick());
    applyEnvVariable(&_workaroundShowAndHideTray, envForceWorkaroundShowAndHideTray());
    applyEnvVariable(&_workaroundManualVisibility, envForceWorkaroundManualVisibility());

    qCInfo(lcApplication) << "Tray menu workarounds:"
                          << "noabouttoshow:" << _workaroundNoAboutToShowUpdate
                          << "fakedoubleclick:" << _workaroundFakeDoubleClick
                          << "showhide:" << _workaroundShowAndHideTray
                          << "manualvisibility:" << _workaroundManualVisibility;


    connect(&_delayedTrayUpdateTimer, &QTimer::timeout, this, &ownCloudGui::updateContextMenu);
    _delayedTrayUpdateTimer.setInterval(2s);
    _delayedTrayUpdateTimer.setSingleShot(true);

    connect(_contextMenu.data(), &QMenu::aboutToShow, this, &ownCloudGui::slotContextMenuAboutToShow);
    // unfortunately aboutToHide is unreliable, it seems to work on OSX though
    connect(_contextMenu.data(), &QMenu::aboutToHide, this, &ownCloudGui::slotContextMenuAboutToHide);

    // Populate the context menu now.
    updateContextMenu();
}

void ownCloudGui::updateContextMenu()
{
    // If it's visible, we can't update live, and it won't be updated lazily: reschedule
    if (contextMenuVisible() && !updateWhileVisible() && _workaroundNoAboutToShowUpdate) {
        if (!_delayedTrayUpdateTimer.isActive()) {
            _delayedTrayUpdateTimer.start();
        }
        return;
    }

    if (_workaroundShowAndHideTray) {
        // To make tray menu updates work with these bugs (see setupContextMenu)
        // we need to hide and show the tray icon. We don't want to do that
        // while it's visible!
        if (contextMenuVisible()) {
            if (!_delayedTrayUpdateTimer.isActive()) {
                _delayedTrayUpdateTimer.start();
            }
            return;
        }
        _tray->hide();
    }

    _contextMenu->clear();
    slotRebuildRecentMenus();

    // We must call deleteLater because we might be called from the press in one of the actions.
    for (auto *menu : std::as_const(_accountMenus)) {
        menu->deleteLater();
    }
    _accountMenus.clear();

    const auto &accountList = AccountManager::instance()->accounts();

    bool isConfigured = (!accountList.isEmpty());
    bool atLeastOneConnected = false;
    bool atLeastOnePaused = false;
    for (const auto &a : accountList) {
        if (a->isConnected()) {
            atLeastOneConnected = true;
            break;
        }
    }

    for (auto *f : FolderMan::instance()->folders()) {
        if (f->syncPaused()) {
            atLeastOnePaused = true;
            break;
        }
    }

    _contextMenu->addAction(Theme::instance()->applicationIcon(), tr("Show %1").arg(Theme::instance()->appNameGUI()), this, &ownCloudGui::slotShowSettings);
    _contextMenu->addSeparator();

    if (accountList.isEmpty()) {
        _contextMenu->addAction(tr("Create a new account"), this, &ownCloudGui::runNewAccountWizard);
    } else {
        if (atLeastOnePaused) {
            _contextMenu->addAction(tr("Resume synchronization"), this, [this] { setPauseOnAllFoldersHelper(AccountManager::instance()->accounts(), false); });
        } else {
            _contextMenu->addAction(tr("Stop synchronization"), this, [this] { setPauseOnAllFoldersHelper(AccountManager::instance()->accounts(), true); });
        }
        _contextMenu->addSeparator();

        // submenus for accounts
        for (const auto &account : accountList) {
            QMenu *accountMenu = new QMenu(account->account()->displayNameWithHost(), _contextMenu.data());
            _accountMenus.append(accountMenu);
            _contextMenu->addMenu(accountMenu);

            addAccountContextMenu(account, accountMenu);
        }
    }

    _contextMenu->addSeparator();

    _contextMenu->addAction(_actionStatus);
    if (isConfigured && atLeastOneConnected) {
        _contextMenu->addMenu(_recentActionsMenu);
    }

    _contextMenu->addSeparator();

    if (_app->debugMode()) {
        auto *debugMenu = _contextMenu->addMenu(QStringLiteral("Debug actions"));
        debugMenu->addAction(QStringLiteral("Crash if asserts enabled - OC_ENSURE"), _app, [] {
            if (OC_ENSURE(false)) {
                Q_UNREACHABLE();
            }
        });
        debugMenu->addAction(QStringLiteral("Crash if asserts enabled - Q_ASSERT"), _app, [] { Q_ASSERT(false); });
        debugMenu->addAction(QStringLiteral("Crash now - Utility::crash()"), _app, [] { Utility::crash(); });
        debugMenu->addAction(QStringLiteral("Crash now - OC_ENFORCE()"), _app, [] { OC_ENFORCE(false); });
        debugMenu->addAction(QStringLiteral("Crash now - qFatal"), _app, [] { qFatal("la Qt fatale"); });
        debugMenu->addAction(QStringLiteral("Restart now"), _app, [] { RestartManager::requestRestart(); });
        debugMenu->addSeparator();
        auto captivePortalCheckbox = debugMenu->addAction(QStringLiteral("Behind Captive Portal"));
        captivePortalCheckbox->setCheckable(true);
        captivePortalCheckbox->setChecked(NetworkInformation::instance()->isForcedCaptivePortal());
        connect(captivePortalCheckbox, &QAction::triggered, [](bool checked) { NetworkInformation::instance()->setForcedCaptivePortal(checked); });
    }

    _contextMenu->addSeparator();

    if (!Theme::instance()->helpUrl().isEmpty()) {
        _contextMenu->addAction(tr("Help"), this, &ownCloudGui::slotHelp);
    }

    if (! Theme::instance()->about().isEmpty()) {
        _contextMenu->addAction(tr("About %1").arg(Theme::instance()->appNameGUI()), this, &ownCloudGui::slotAbout);
    }

    _contextMenu->addAction(tr("Quit %1").arg(Theme::instance()->appNameGUI()), _app, &QApplication::quit);

    if (_workaroundShowAndHideTray) {
        _tray->show();
    }
}

void ownCloudGui::updateContextMenuNeeded()
{
    // if it's visible and we can update live: update now
    if (contextMenuVisible() && updateWhileVisible()) {
        // Note: don't update while visible on OSX
        // https://bugreports.qt.io/browse/QTBUG-54845
        updateContextMenu();
        return;
    }

    // if we can't lazily update: update later
    if (_workaroundNoAboutToShowUpdate) {
        // Note: don't update immediately even in the invisible case
        // as that can lead to extremely frequent menu updates
        if (!_delayedTrayUpdateTimer.isActive()) {
            _delayedTrayUpdateTimer.start();
        }
        return;
    }
}

void ownCloudGui::slotShowTrayMessage(const QString &title, const QString &msg, const QIcon &icon)
{
    _tray->showMessage(title, msg, icon.isNull() ? Resources::getCoreIcon(QStringLiteral("states/information")) : icon);
}

void ownCloudGui::slotShowOptionalTrayMessage(const QString &title, const QString &msg, const QIcon &icon)
{
    ConfigFile cfg;
    if (cfg.optionalDesktopNotifications()) {
        slotShowTrayMessage(title, msg, icon);
    }
}


/*
 * open the folder with the given Alias
 */
void ownCloudGui::slotFolderOpenAction(Folder *f)
{
    if (f) {
        qCInfo(lcApplication) << "opening local URL " << f->path();
        QUrl url = QUrl::fromLocalFile(f->path());

#ifdef Q_OS_WIN
        // work around a bug in QDesktopServices on Win32, see i-net
        QString filePath = f->path();

        if (filePath.startsWith(QLatin1String("\\\\")) || filePath.startsWith(QLatin1String("//")))
            url = QUrl::fromLocalFile(QDir::toNativeSeparators(filePath));
        else
            url = QUrl::fromLocalFile(filePath);
#endif
        QDesktopServices::openUrl(url);
    }
}

void ownCloudGui::setupActions()
{
    _actionStatus = new QAction(tr("Unknown status"), this);
    _actionStatus->setEnabled(false);
}

void ownCloudGui::slotRebuildRecentMenus()
{
    _recentActionsMenu->clear();
    if (!_recentItemsActions.isEmpty()) {
        for (auto *a : std::as_const(_recentItemsActions)) {
            _recentActionsMenu->addAction(a);
        }
        _recentActionsMenu->addSeparator();
    } else {
        _recentActionsMenu->addAction(tr("No items synced recently"))->setEnabled(false);
    }
    // add a more... entry.
    _recentActionsMenu->addAction(tr("Details..."), this, &ownCloudGui::slotShowSyncProtocol);
}

/// Returns true if the completion of a given item should show up in the
/// 'Recent Activity' menu
static bool shouldShowInRecentsMenu(const SyncFileItem &item)
{
    return !Progress::isIgnoredKind(item._status) && item.instruction() != CSYNC_INSTRUCTION_NONE;
}

void ownCloudGui::slotUpdateProgress(Folder *folder, const ProgressInfo &progress)
{
    if (progress.status() == ProgressInfo::Discovery) {
        if (!progress._currentDiscoveredRemoteFolder.isEmpty()) {
            _actionStatus->setText(tr("Checking for changes in remote '%1'")
                                       .arg(progress._currentDiscoveredRemoteFolder));
        } else if (!progress._currentDiscoveredLocalFolder.isEmpty()) {
            _actionStatus->setText(tr("Checking for changes in local '%1'")
                                       .arg(progress._currentDiscoveredLocalFolder));
        }
    } else if (progress.status() == ProgressInfo::Done) {
        QTimer::singleShot(2s, this, &ownCloudGui::slotComputeOverallSyncStatus);
    }
    if (progress.status() != ProgressInfo::Propagation) {
        return;
    }

    if (progress.totalSize() == 0) {
        qint64 currentFile = progress.currentFile();
        qint64 totalFileCount = qMax(progress.totalFiles(), currentFile);
        QString msg;
        if (progress.trustEta()) {
            msg = tr("Syncing %1 of %2  (%3 left)")
                      .arg(currentFile)
                      .arg(totalFileCount)
                      .arg(Utility::durationToDescriptiveString2(std::chrono::milliseconds(progress.totalProgress().estimatedEta)));
        } else {
            msg = tr("Syncing %1 of %2")
                      .arg(currentFile)
                      .arg(totalFileCount);
        }
        _actionStatus->setText(msg);
    } else {
        QString totalSizeStr = Utility::octetsToString(progress.totalSize());
        QString msg;
        if (progress.trustEta()) {
            msg = tr("Syncing %1 (%2 left)")
                      .arg(totalSizeStr, Utility::durationToDescriptiveString2(std::chrono::milliseconds(progress.totalProgress().estimatedEta)));
        } else {
            msg = tr("Syncing %1")
                      .arg(totalSizeStr);
        }
        _actionStatus->setText(msg);
    }

    if (!progress._lastCompletedItem.isEmpty() && shouldShowInRecentsMenu(progress._lastCompletedItem)) {
        QString kindStr = Progress::asResultString(progress._lastCompletedItem);
        QString timeStr = QTime::currentTime().toString(QStringLiteral("hh:mm"));
        QString actionText = tr("%1 (%2, %3)").arg(progress._lastCompletedItem._file, kindStr, timeStr);
        QAction *action = new QAction(actionText, this);
        QString fullPath = folder->path() + QLatin1Char('/') + progress._lastCompletedItem._file;
        if (QFile(fullPath).exists()) {
            connect(action, &QAction::triggered, this, [this, fullPath] { this->slotOpenPath(fullPath); });
        } else {
            action->setEnabled(false);
        }
        if (_recentItemsActions.length() > 5) {
            _recentItemsActions.takeFirst()->deleteLater();
        }
        _recentItemsActions.append(action);

        // Update the "Recent" menu if the context menu is being shown,
        // otherwise it'll be updated later, when the context menu is opened.
        if (updateWhileVisible() && contextMenuVisible()) {
            slotRebuildRecentMenus();
        }
    }
}

// todo: #28
void ownCloudGui::runNewAccountWizard()
{
    if (_wizardController.isNull()) {
        // passing the settings dialog as parent makes sure the wizard will be shown above it
        // as the settingsDialog's lifetime spans across the entire application but the dialog will live much shorter,
        // we have to clean it up manually when finished() is emitted
        _wizardController = new Wizard::SetupWizardController(settingsDialog());

        connect(_wizardController, &Wizard::SetupWizardController::finished, this,
            [this](AccountPtr newAccount, Wizard::SyncMode syncMode, const QVariantMap &dynamicRegistrationData) {
                // note: while the wizard is shown, we disable the folder synchronization
                // previously we could perform this just here, but now we have to postpone this depending on whether selective sync was chosen
                // see also #9497

                // when the dialog is closed before it has finished, there won't be a new account to set up
                // the wizard controller signalizes this by passing a null pointer
                if (!newAccount.isNull()) {
                    // finally, call the slot that finalizes the setup
                    auto accountStatePtr = ocApp()->addNewAccount(newAccount);
                    accountStatePtr->setSettingUp(true);

                    _settingsDialog->setCurrentAccount(accountStatePtr->account().data());

                    // ensure we are connected and fetch the capabilities
                    // refactoring todo: WHAT is this doing here? we have a "legit" account state pointer. it has a checkConnection
                    // that can do this job
                    auto validator = new ConnectionValidator(accountStatePtr->account(), accountStatePtr->account().data());

                    // todo: #27 - we already get the capabilities in the last check of the url to filter out oc10 accounts. pass that info along instead of
                    // fetching it again also again, none of this belongs here. The account manager should deal with it.
                    QObject::connect(validator, &ConnectionValidator::connectionResult, accountStatePtr.data(),
                        [accountStatePtr, syncMode, dynamicRegistrationData, validator, this](ConnectionValidator::Status status, const QStringList &) {
                            switch (status) {
                            // a server we no longer support but that might work
                            case ConnectionValidator::ServerVersionMismatch:
                                [[fallthrough]];
                            case ConnectionValidator::Connected: {
                                // saving once after adding makes sure the account is stored in the config in a working state
                                // this is needed to ensure a consistent state in the config file upon unexpected terminations of the client
                                // (for instance, when running from a debugger and stopping the process from there)
                                AccountManager::instance()->save(true);

                                // only now, we can store the dynamic registration data in the keychain
                                if (!dynamicRegistrationData.isEmpty()) {
                                    OAuth::saveDynamicRegistrationDataForAccount(accountStatePtr->account(), dynamicRegistrationData);
                                }

                                // the account is now ready, emulate a normal account loading and Q_EMIT that the credentials are ready
                                // Refactoring todo: no. the account should emit this when it meets some internal state, not the gui controller!!!!
                                Q_EMIT accountStatePtr->account()->credentialsFetched();

                                switch (syncMode) {
                                case Wizard::SyncMode::SyncEverything:
                                case Wizard::SyncMode::UseVfs: {
                                    bool useVfs = syncMode == Wizard::SyncMode::UseVfs;
                                    // Refactoring example: don't handle complicated stuff locally, REQUEST that it be performed by some entity
                                    // more properly responsible for the task
                                    Q_EMIT requestSetUpSyncFoldersForAccount(accountStatePtr, useVfs);
                                    accountStatePtr->setSettingUp(false);
                                    break;
                                }
                                case Wizard::SyncMode::ConfigureUsingFolderWizard: {
                                    Q_ASSERT(!accountStatePtr->account()->hasDefaultSyncRoot());

                                    auto *folderWizard = new FolderWizard(accountStatePtr, ocApp()->gui()->settingsDialog());
                                    folderWizard->setAttribute(Qt::WA_DeleteOnClose);

                                    // TODO: duplication of AccountSettings
                                    // adapted from AccountSettings::slotFolderWizardAccepted()
                                    // Refactoring todo: the way to fix this is to have a single controller dedicated to the FolderWizard, which will handle
                                    // signals from that gui regardless of where it's "installed".
                                    connect(folderWizard, &QDialog::accepted, this, [accountStatePtr, folderWizard]() {
                                        FolderMan *folderMan = FolderMan::instance();

                                        qCInfo(lcApplication) << "Folder wizard completed";
                                        auto config = folderWizard->result();

                                        // The gui should not allow users to selectively choose any sync lists if vfs is enabled, but this kind of check was
                                        // originally in play here so...keep it just in case.
                                        if (config.useVirtualFiles && !config.selectiveSyncBlackList.empty()) {
                                            config.selectiveSyncBlackList.clear();
                                        }

                                        folderMan->addFolderFromGui(accountStatePtr, config);

                                        accountStatePtr->setSettingUp(false);
                                    });

                                    connect(folderWizard, &QDialog::rejected, [accountStatePtr]() {
                                        qCInfo(lcApplication) << "Folder wizard cancelled";
                                        accountStatePtr->setSettingUp(false);
                                    });

                                    ocApp()
                                        ->gui()
                                        ->settingsDialog()
                                        ->accountSettings(accountStatePtr->account().get())
                                        ->addModalLegacyDialog(folderWizard, AccountSettings::ModalWidgetSizePolicy::Expanding);
                                    break;
                                }
                                case OCC::Wizard::SyncMode::Invalid:
                                    Q_UNREACHABLE();
                                }
                            }
                            case ConnectionValidator::ClientUnsupported:
                                break;
                            default:
                                Q_UNREACHABLE();
                            }
                            validator->deleteLater();
                        });

                    validator->checkServer();
                }

                // make sure the wizard is cleaned up eventually
                _wizardController->deleteLater();
            });

        // all we have to do is show the dialog...
        ocApp()->gui()->settingsDialog()->addModalWidget(_wizardController->window());
    }
}

void ownCloudGui::setPauseOnAllFoldersHelper(const QList<AccountStatePtr> &accounts, bool pause)
{
    for (auto *f : FolderMan::instance()->folders()) {
        if (accounts.contains(f->accountState())) {
            f->setSyncPaused(pause);
            if (pause) {
                f->slotTerminateSync(tr("User paused sync for account '%1'").arg(f->accountState()->account()->displayNameWithHost()));
            }
        }
    }
}

void ownCloudGui::slotShowSettings()
{
    raise();
}

void ownCloudGui::slotShowSyncProtocol()
{
    slotShowSettings();
    _settingsDialog->setCurrentPage(SettingsDialog::SettingsPage::Activity);
}


void ownCloudGui::slotShutdown()
{
    // explicitly close windows. This is somewhat of a hack to ensure
    // that saving the geometries happens ASAP during a OS shutdown

    // those do delete on close
    _settingsDialog->close();
}

void ownCloudGui::slotToggleLogBrowser()
{
    auto logBrowser = new LogBrowser(settingsDialog());
    logBrowser->setAttribute(Qt::WA_DeleteOnClose);
    ownCloudGui::raise();
    logBrowser->open();
}

void ownCloudGui::slotHelp()
{
    QDesktopServices::openUrl(QUrl(Theme::instance()->helpUrl()));
}

void ownCloudGui::raise()
{
    auto window = ocApp()->gui()->settingsDialog();
    window->show();
    window->raise();
    window->activateWindow();

#if defined(Q_OS_WIN)
    // Windows disallows raising a Window when you're not the active application.
    // Use a common hack to attach to the active application
    const auto activeProcessId = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
    if (activeProcessId != qApp->applicationPid()) {
        const auto threadId = GetCurrentThreadId();
        // don't step here with a debugger...
        if (AttachThreadInput(threadId, activeProcessId, true))
        {
            const auto hwnd = reinterpret_cast<HWND>(window->winId());
            SetForegroundWindow(hwnd);
            SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            AttachThreadInput(threadId, activeProcessId, false);
        }
    }
#endif
}


void ownCloudGui::slotShowShareDialog(const QString &sharePath, const QString &localPath, ShareDialogStartPage startPage)
{
    QString file;
    const auto folder = FolderMan::instance()->folderForPath(localPath, &file);
    if (!folder) {
        qCWarning(lcApplication) << "Could not open share dialog for" << localPath << "no responsible folder found";
        return;
    }
    if (folder->accountState()->account()->capabilities().filesSharing().sharing_roles) {
        fetchPrivateLinkUrl(folder->accountState()->account(), folder->webDavUrl(), sharePath, this, [](const QUrl &url) {
            const auto queryUrl = Utility::concatUrlPath(url, QString(), {{QStringLiteral("details"), QStringLiteral("sharing")}});
            Utility::openBrowser(queryUrl, nullptr);
        });
    } else {
        // oC10 code path
        const auto accountState = folder->accountState();

        SyncJournalFileRecord fileRecord;

        bool resharingAllowed = true; // lets assume the good
        if (folder->journalDb()->getFileRecord(file, &fileRecord) && fileRecord.isValid()) {
            // check the permission: Is resharing allowed?
            if (!fileRecord._remotePerm.isNull() && !fileRecord._remotePerm.hasPermission(RemotePermissions::CanReshare)) {
                resharingAllowed = false;
            }
        }

        // As a first approximation, set the set of permissions that can be granted
        // either to everything (resharing allowed) or nothing (no resharing).
        //
        // The correct value will be found with a propfind from ShareDialog.
        // (we want to show the dialog directly, not wait for the propfind first)
        SharePermissions maxSharingPermissions =
            SharePermissionRead
            | SharePermissionUpdate | SharePermissionCreate | SharePermissionDelete
            | SharePermissionShare;
        if (!resharingAllowed) {
            maxSharingPermissions = SharePermission(0);
        }

        if (_shareDialog && _shareDialog->localPath() == localPath) {
            qCInfo(lcApplication) << "A share dialog for this path already exists" << sharePath << localPath;
            // There might be another modal dialog on top, but that is usually more important (e.g. a login dialog).
            raise();
        } else {
            qCInfo(lcApplication) << "Opening new share dialog" << sharePath << localPath << maxSharingPermissions;
            if (_shareDialog) {
                _shareDialog->close();
            }
            _shareDialog = new ShareDialog(accountState, folder->webDavUrl(), sharePath, localPath, maxSharingPermissions, startPage, settingsDialog());
            _shareDialog->setAttribute(Qt::WA_DeleteOnClose, true);
            ocApp()
                ->gui()
                ->settingsDialog()
                ->accountSettings(accountState->account().get())
                ->addModalLegacyDialog(_shareDialog, AccountSettings::ModalWidgetSizePolicy::Expanding);
        }
    }
}

void ownCloudGui::slotAbout()
{
    if(!_aboutDialog) {
        _aboutDialog = new AboutDialog(_settingsDialog);
        _aboutDialog->setAttribute(Qt::WA_DeleteOnClose);
        ocApp()->gui()->settingsDialog()->addModalWidget(_aboutDialog);
    }
}


} // end namespace
