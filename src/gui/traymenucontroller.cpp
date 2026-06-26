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

#include "traymenucontroller.h"
#include "account.h"
#include "accountmanager.h"
#include "accountstate.h"
#include "application.h"
#include "common/restartmanager.h"
#include "common/syncjournalfilerecord.h"
#include "configfile.h"
#include "folderman.h"
#include "gui/networkinformation.h"
#include "guiutility.h"
#include "libsync/theme.h"
#include "logbrowser.h"
#include "mainwindow/mainwindow.h"
#include "progressdispatcher.h"

#include "resources/resources.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QMessageBox>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

using namespace std::chrono_literals;

namespace OCC {

// simple helper to compute the overall status for the tray icon
SyncResult::Status trayOverallStatus()
{
    TrayOverallStatusResult result;
    for (auto *folder : FolderMan::instance()->folders()) {
        result.addResult(folder);
    }
    if (result.overallStatus().status() == SyncResult::Undefined) {
        return SyncResult::Offline;
    }
    return result.overallStatus().status();
}

TrayMenuController::TrayMenuController(Application *parent)
    : QObject(parent)
    , _tray(new QSystemTrayIcon(this))
    , _app(parent)
{

    connect(_tray, &QSystemTrayIcon::activated, this, &TrayMenuController::slotTrayClicked);

    setupTrayContextMenu();

    // init systray
    slotComputeOverallSyncStatus();
    _tray->show();

    // Refactoring todo: we have a very nasty habit of connecting to globally available managers, etc inside constructors or similar.
    // this should only happen when we are dealing with a formal dependency: eg a dep passed by injection and kept as member,
    // or a dep that is instantiated inside the class (also with a member kept)
    // in cases like this one, the external deps should be instantiated externally and connected externally. This is normally part
    // of an app building routine. The global singletons have to go and this is an important step to achieving that.
    FolderMan *folderMan = FolderMan::instance();
    connect(folderMan, &FolderMan::folderSyncStateChange, this, &TrayMenuController::slotSyncStateChange);
}

TrayMenuController::~TrayMenuController()
{
}

void TrayMenuController::slotTrayClicked(QSystemTrayIcon::ActivationReason reason)
{
    // Left click
    if (reason == QSystemTrayIcon::Trigger) {
        // this covers left click - on mac it shows the menu so we don't want to do anything
        // more. on windows (and linux, presumably, given the original code) we just show the
        // main window on left click.
        if (!Utility::isMac())
            ocApp()->ensureVisible();
    }
}

void TrayMenuController::slotSyncStateChange(Folder *folder)
{
    slotComputeOverallSyncStatus();

    if (!folder) {
        return; // Valid, just a general GUI redraw was needed.
    }

    auto result = folder->syncResult();

    qCInfo(lcApplication) << "Sync state changed for folder " << folder->remoteUrl().toString() << ": " << Utility::enumToDisplayName(result.status());
}

void TrayMenuController::slotTrayMessageIfServerUnsupported(Account *account)
{
    if (account->serverSupportLevel() != Account::ServerSupportLevel::Supported) {
        slotShowTrayMessage(tr("Unsupported Server Version"),
            tr("The server on account %1 runs an unsupported version %2. "
               "Using this client with unsupported server versions is untested and "
               "potentially dangerous. Proceed at your own risk.")
                .arg(account->displayNameWithHost(), account->capabilities().status().versionString()));
    }
}

QIcon TrayMenuController::getTrayStatusIcon(const SyncResult::Status &status) const
{
    auto contextMenuVisible = _tray->contextMenu() && _tray->contextMenu()->isVisible();
    return Theme::instance()->themeTrayIcon(SyncResult{status}, contextMenuVisible);
}

void TrayMenuController::slotComputeOverallSyncStatus()
{
    auto status = trayOverallStatus();
    const QIcon statusIcon = getTrayStatusIcon(status);
    _tray->setIcon(statusIcon);
}

void TrayMenuController::setupTrayContextMenu()
{
    // using the main window as parent for memory management
    // todo: review this as for some reason I remember there is some other way it's supposed to be done
    auto menu = new QMenu(ocApp()->mainWindow());
    menu->setTitle(Theme::instance()->appNameGUI());

    _tray->setContextMenu(menu);

    // Populate the context menu now.
    menu->addAction(Theme::instance()->applicationIcon(), tr("Show %1").arg(Theme::instance()->appNameGUI()), ocApp(), &Application::ensureVisible);
    menu->addSeparator();

    if (_app->debugMode()) {
        auto *debugMenu = menu->addMenu(QStringLiteral("Debug actions"));
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

        menu->addSeparator();
    }

    if (!Theme::instance()->helpUrl().isEmpty()) {
        menu->addAction(tr("Help"), this, &TrayMenuController::requestShowHelp);
    }

    if (! Theme::instance()->about().isEmpty()) {
        menu->addAction(tr("About %1").arg(Theme::instance()->appNameGUI()), this, &TrayMenuController::requestShowAbout);
    }

    menu->addAction(tr("Quit %1").arg(Theme::instance()->appNameGUI()), _app, &QApplication::quit);
}

void TrayMenuController::slotShowTrayMessage(const QString &title, const QString &msg, const QIcon &icon)
{
    _tray->showMessage(title, msg, icon.isNull() ? Resources::getCoreIcon(QStringLiteral("states/information")) : icon);
}

void TrayMenuController::slotShowOptionalTrayMessage(const QString &title, const QString &msg, const QIcon &icon)
{
    ConfigFile cfg;
    if (cfg.optionalDesktopNotifications()) {
        slotShowTrayMessage(title, msg, icon);
    }
}

void TrayMenuController::slotShowShareInBrowser(const QString &sharePath, const QString &localPath)
{
    QString file;
    const auto folder = FolderMan::instance()->folderForPath(localPath, &file);
    if (!folder) {
        qCWarning(lcApplication) << "Could not open browser share page for" << localPath << "no responsible folder found";
        return;
    }

    if (folder->accountState()->account()->capabilities().filesSharing().sharing_roles) {
        fetchPrivateLinkUrl(folder->accountState()->account(), folder->webDavUrl(), sharePath, this, [](const QUrl &url) {
            const auto queryUrl = Utility::concatUrlPath(url, QString(), {{QStringLiteral("details"), QStringLiteral("sharing")}});
            Utility::openBrowser(queryUrl, nullptr);
        });
    }
}


} // end namespace
