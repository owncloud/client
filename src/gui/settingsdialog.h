/*
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

#pragma once

#include "accountstate.h"
#include "gui/qmlutils.h"
#include "owncloudgui.h"
#include "progressdispatcher.h"

#include <QMainWindow>
#include <QStyledItemDelegate>

#define USE_NEW_FOLDER_LIST

namespace OCC {

namespace Ui {
    class SettingsDialog;
}
class AccountView;
class AccountSettings;
class Application;
class FolderMan;
class ownCloudGui;
class ActivitySettings;
class GeneralSettings;

/**
 * @brief The SettingsDialog class
 * @ingroup gui
 */
class SettingsDialog : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(SettingsPage currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(Account *currentAccount READ currentAccount WRITE setCurrentAccount NOTIFY currentAccountChanged)
    QML_ELEMENT
    QML_UNCREATABLE("C++ only")
    OC_DECLARE_WIDGET_FOCUS
public:
    enum class SettingsPage { None, Activity, Settings, Account };
    Q_ENUM(SettingsPage)
    explicit SettingsDialog(ownCloudGui *gui, QWidget *parent = nullptr);
    ~SettingsDialog() override;

    void addModalWidget(QWidget *w);

    void requestModality(Account *account);
    void ceaseModality(Account *account);

    AccountSettings *accountSettings(Account *account) const;
    AccountView *accountView(Account *account) const;

    SettingsPage currentPage() const;

    void setCurrentPage(SettingsPage currentPage);

    void setCurrentAccount(Account *account);

    // todo: #37 - this is stickier to get rid of as it's used in the qml related to the account. That needs to go.
    Account *currentAccount() const;

public Q_SLOTS:
    // this is a direct call from QML
    void createNewAccount();
    void runFolderWizard(Account *account);


Q_SIGNALS:
    void currentPageChanged();
    // I think this only goes to qml
    void currentAccountChanged();

protected Q_SLOTS:
    void onAccountAdded(AccountState *state);
    void onAccountRemoved(AccountState *state);

protected:
    void setVisible(bool visible) override;


private:
    Ui::SettingsDialog *const _ui;

    // todo: #37
    QHash<QUuid, AccountSettings *> _widgetForAccount;
    QHash<QUuid, AccountView *> _viewForAccount;

    ActivitySettings *_activitySettings;
    ownCloudGui *_gui;
    QList<Account *> _modalStack;

    GeneralSettings *_generalSettings;
    SettingsPage _currentPage = SettingsPage::None;
    // todo: #37
    Account *_currentAccount = nullptr;
};
}
