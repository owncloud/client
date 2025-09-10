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

#pragma once

#include "account.h"
#include "gui/owncloudguilib.h"
#include "progressdispatcher.h"

#include <QObject>
#include <QPointer>
#include <QMenu>
#include <QSystemTrayIcon>

namespace OCC {

namespace Wizard {
    class SetupWizardController;
}
class Folder;

class AboutDialog;
class SettingsDialog;
class ShareDialog;
class Application;
class LogBrowser;

enum class ShareDialogStartPage {
    UsersAndGroups,
    PublicLinks,
};

/**
 * @brief The ownCloudGui class
 * @ingroup gui
 */
class OWNCLOUDGUI_EXPORT ownCloudGui : public QObject
{
    Q_OBJECT
public:
    explicit ownCloudGui(Application *parent = nullptr);
    ~ownCloudGui() override;

    /**
     * Raises our main Window to the front with the raiseWidget in focus.
     * If raiseWidget is a dialog and not visible yet, ->open will be called.
     * For normal widgets we call showNormal.
     */
    static void raise();

    /// Whether the tray menu is visible
    bool contextMenuVisible() const;

    SettingsDialog *settingsDialog() const;

    void runAccountWizard();

Q_SIGNALS:
    void requestSetUpSyncFoldersForAccount(AccountState *account, bool useVfs);

public Q_SLOTS:
    void setupContextMenu();
    void slotComputeOverallSyncStatus();
    void slotShowTrayMessage(const QString &title, const QString &msg, const QIcon &icon = {});
    void slotShowOptionalTrayMessage(const QString &title, const QString &msg, const QIcon &icon = {});
    void slotUpdateProgress(Folder *folder, const ProgressInfo &progress);
    void slotFoldersChanged();
    void slotShowSettings();
    void slotShutdown();
    void slotSyncStateChange(Folder *);
    void slotTrayClicked(QSystemTrayIcon::ActivationReason reason);
    void slotToggleLogBrowser();
    void slotOpenSettingsDialog();
    void slotHelp();
    void slotAbout();
    void slotAccountStateChanged();
    void slotTrayMessageIfServerUnsupported(Account *account);

    /**
     * Open a share dialog for a file or folder.
     *
     * sharePath is the full remote path to the item,
     * localPath is the absolute local path to it (so not relative
     * to the folder).
     */
    void slotShowShareDialog(const QString &sharePath, const QString &localPath, ShareDialogStartPage startPage);
    void handleAccountSetupError(const QString &error);

private:

    QSystemTrayIcon *_tray;
    SettingsDialog *_settingsDialog;
    QPointer<ShareDialog> _shareDialog;

    Application *_app;

    // keeping a pointer on those dialogs allows us to make sure they will be shown only once
    QPointer<Wizard::SetupWizardController> _wizardController;
    QPointer<AboutDialog> _aboutDialog;
};

} // namespace OCC
