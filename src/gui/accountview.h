/*
 * Copyright (C) by Lisa Reese <lisa.reese@kiteworks.com>
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

#include "folder.h"
#include "gui/qmlutils.h"
#include "owncloudgui.h"
#include "progressdispatcher.h"

#include <QSortFilterProxyModel>
#include <QWidget>

class QModelIndex;
class QNetworkReply;
class QLabel;
class QStandardItemModel;

namespace OCC {
class AccountModalWidget;

namespace Ui {
    class AccountView;
}

class FolderMan;

class Account;
class AccountState;
// class FolderStatusModel;
class FolderStatusDelegate;

/**
 * @brief The AccountView class
 * @ingroup gui
 */
class OWNCLOUDGUI_EXPORT AccountView : public QWidget
{
    Q_OBJECT

public:
    enum class ModalWidgetSizePolicy { Minimum = QSizePolicy::Minimum, Expanding = QSizePolicy::Expanding };
    Q_ENUM(ModalWidgetSizePolicy)

    explicit AccountView(AccountState *accountState, QWidget *parent = nullptr);
    ~AccountView() override;

    void addModalAccountWidget(AccountModalWidget *widget);

    // this is called by SettingsDialog directly but should be corrected to either respond to signal, or just make
    // it a normal function
    void slotAddFolder();


protected slots:
    void slotAccountStateChanged(AccountState::State state);
    void slotDeleteAccount();
    void slotOpenAccountInBrowser();
    void slotToggleSignInState();
    void slotFolderWizardAccepted();
    void onRequestShowModalWidget(QWidget *widget) { addModalLegacyDialog(widget, ModalWidgetSizePolicy::Expanding); }
    void onRequestAccountModalWidget(OCC::AccountModalWidget *widget);

protected:
    void accountSettingUpChanged(bool settingUp);
    void addModalLegacyDialog(QWidget *widget, ModalWidgetSizePolicy sizePolicy);

    void showEvent(QShowEvent *ev) override;


    // private slots:


private:

    enum class StatusIcon { None, Connected, Disconnected, Info, Warning };
    void showConnectionLabel(const QString &message, StatusIcon statusIcon, QStringList errors = QStringList());


    void buildManageAccountMenu();

    Ui::AccountView *ui;

    QStandardItemModel *_model;
    QSortFilterProxyModel *_sortModel;
    bool _wasDisabledBefore;
    QPointer<AccountState> _accountState;
    // are we already in the destructor
    bool _goingDown = false;
    uint _syncedSpaces = 0;
    uint _unsyncedSpaces = 0;
};

} // namespace OCC
