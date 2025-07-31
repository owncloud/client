/*
 * Copyright (C) by Olivier Goffart <ogoffart@woboq.com>
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

#include "account.h"
#include "accountstate.h"
#include "newaccountwizard/newaccountmodel.h"

#include <QtQmlIntegration/QtQmlIntegration>


class QJSEngine;
class QQmlEngine;
namespace OCC {

/**
   @brief The AccountManager class
   @ingroup gui
*/
class OWNCLOUDGUI_EXPORT AccountManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<AccountState *> accounts READ accountsRaw() NOTIFY accountsChanged)
    QML_SINGLETON
    QML_ELEMENT
public:
    static AccountManager *instance();

    static AccountManager *create(QQmlEngine *qmlEngine, QJSEngine *);
    ~AccountManager() override {}

    /**
     * Saves the accounts to a given settings file
     */
    void save(bool saveCredentials = true);

    /**
     * @brief createAccount creates an account from scratch
     * @param model contains the data used to set up the new account (usually collected from the new account wizard)
     * @return the AccountPtr associated with the new account. for now.
     */
    AccountPtr createAccount(const NewAccountModel &model);

    /**
     * Creates account objects from a given settings file.
     *
     * Returns false if there was an error reading the settings,
     * but note that settings not existing is not an error.
     */
    bool restore();

    /**
     * Add this account in the list of saved accounts.
     * Typically called from the wizard
     */
    // todo: #28 - this should not be public and should not be called directly by any third party
    AccountStatePtr addAccount(const AccountPtr &newAccount);

    /**
     * remove all accounts
     */
    void shutdown();

    /**
     * Return a list of all accounts.
     */
    const QList<AccountStatePtr> accounts() const { return _accounts.values(); }

    /**
     * Return the account state pointer for an account identified by its display name
     */
    Q_DECL_DEPRECATED_X("Please use the uuid to specify the account") AccountStatePtr account(const QString &name);

    /**
     * Return the accountState state pointer for an accountState identified by its display name
     */
    AccountStatePtr accountState(const QUuid uuid);

    /**
     * Delete the AccountState
     */
    void deleteAccount(AccountStatePtr account);


    /**
     * Creates an account and sets up some basic handlers.
     * Does *not* add the account to the account manager just yet.
     */

    // todo: #28 - this has to be updated - it only works in tandem with the loadAccountHelper and should not even
    //  be public. also, any create account scheme *should* register the new instance in the account manager
    static AccountPtr createAccount(const QUuid &uuid);

    /**
     * Returns a sorted list of displayNames
     */
    QStringList accountNames() const;

    bool accountForLoginExists(const QUrl &url, const QString &davUser) const;

private:
    // expose raw pointers to qml
    QList<AccountState *> accountsRaw() const;

    // saving and loading Account to settings
    void saveAccountHelper(Account *account, QSettings &settings, bool saveCredentials = true);
    AccountPtr loadAccountHelper(QSettings &settings);

    bool restoreFromLegacySettings();

    bool isAccountIdAvailable(const QString &id) const;
    QString generateFreeAccountId() const;

    // Adds an account to the tracked list, emitting accountAdded()
    AccountStatePtr addAccountState(std::unique_ptr<AccountState> &&accountState);

public Q_SLOTS:
    /// Saves account data, not including the credentials
    void saveAccount(Account *account, bool saveCredentials);

Q_SIGNALS:
    void accountAdded(AccountStatePtr account);
    void accountRemoved(AccountStatePtr account);
    void lastAccountRemoved();

    // this signal is not formally connected anywhere, but it's used on the Q_PROPERTY declared for "accounts" above
    // the accountManager::accounts prop is used as a model in AccountsBar.qml so the accounts are reevaluated in that gui
    // any time the accountsChanged signal is emitted.
    void accountsChanged();

private:
    AccountManager() {}
    QMap<QUuid, AccountStatePtr> _accounts;
    /// Account ids from settings that weren't read
    QSet<QString> _additionalBlockedAccountIds;
};
}
