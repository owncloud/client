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
class AccountFoldersView;

/**
 * @brief The AccountView class
 * @ingroup gui
 */
class OWNCLOUDGUI_EXPORT AccountView : public QWidget
{
    Q_OBJECT

public:
    explicit AccountView(QWidget *parent);
    ~AccountView() override;

    void setAccountMenu(QMenu *menu);
    void setConnectionLabel(const QString &message, const QIcon &icon, QStringList errors = QStringList());

    // this is primarily used to run an account "modal" widget
    void setTopStackWidget(QWidget *widget);
    void removeStackWidget(QWidget *widget);

    // holding my nose for now - this should not be public, nor should the type be "embedded" in the view's ui.
    // todo: replace the concrete folders view with a placeholder location so the controller can SET the folders view in the
    // account view.
    // open question: would this mess up the squish tests? we'll soon learn the answer
    AccountFoldersView *foldersView();
    void accountSettingUpChanged(bool settingUp);

    // this is called by SettingsDialog directly but should be corrected to either respond to signal, or just make
    // it a normal function
    void slotAddFolder();

signals:
    void addFolderClicked();

protected:
    void showEvent(QShowEvent *ev) override;

private:
    Ui::AccountView *_ui;
};

} // namespace OCC

Q_DECLARE_METATYPE(OCC::AccountView)
