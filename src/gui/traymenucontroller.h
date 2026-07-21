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

#include "gui/owncloudguilib.h"
#include "syncresult.h"

#include <QObject>
#include <QPointer>
#include <QSystemTrayIcon>

namespace OCC {

class Account;
class Folder;

class ShareDialog;

/**
 * @brief The ownCloudGui class
 * @ingroup gui
 */
class OWNCLOUDGUI_EXPORT TrayMenuController : public QObject
{
    Q_OBJECT
public:
    explicit TrayMenuController(QObject *parent);
    ~TrayMenuController() override;

Q_SIGNALS:
    void requestShowAbout();
    void requestShowHelp();

public Q_SLOTS:
    void setupTrayContextMenu();
    void slotComputeOverallSyncStatus();
    void slotShowTrayMessage(const QString &title, const QString &msg, const QIcon &icon = {});
    void slotShowOptionalTrayMessage(const QString &title, const QString &msg, const QIcon &icon = {});
    void slotSyncStateChange(Folder *);
    void slotTrayClicked(QSystemTrayIcon::ActivationReason reason);
    void slotTrayMessageIfServerUnsupported(Account *account);

    /**
     * Open a share dialog for a file or folder.
     *
     * sharePath is the full remote path to the item,
     * localPath is the absolute local path to it (so not relative
     * to the folder).
     */
    void slotShowShareInBrowser(const QString &sharePath, const QString &localPath);

private:
    QIcon getTrayStatusIcon(const SyncResult::Status &status) const;

    QSystemTrayIcon *_tray;
    QPointer<ShareDialog> _shareDialog;
};

} // namespace OCC
