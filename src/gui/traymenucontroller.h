/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
