/*
 * Copyright (C) by Krzesimir Nowak <krzesimir@endocode.com>
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

#include <QObject>

#include "accessmanager.h"
#include "accountfwd.h"
#include "owncloudlib.h"
#include <csync.h>

class QNetworkAccessManager;
class QNetworkReply;
namespace OCC {

class AbstractNetworkJob;

class OWNCLOUDSYNC_EXPORT AbstractCredentials : public QObject
{
    Q_OBJECT

public:
    // note the account is also the parent and effective owner of the creds!!!
    AbstractCredentials(Account *account, QObject *parent);
    // No need for virtual destructor - QObject already has one.

    virtual AccessManager *createAccessManager() const = 0;

    /** Whether there are credentials that can be used for a connection attempt. */
    virtual bool ready() const = 0;

    /** Whether fetchFromKeychain() was ever called before. */
    // todo: DC-128 - evaluate the need for this. it's weird.
    bool wasEverFetched() const { return _wasEverFetched; }

    /** Trigger (async) fetching of credential information
     *
     * Should set _wasEverFetched = true, update the ready() state, and fetched() is emitted
     */
    virtual void fetchFromKeychain() = 0;

    /** Ask credentials from the user (typically async)
     *
     * Should Q_EMIT asked() when done.
     */
    virtual void askFromUser() = 0;

    // todo: this is an unholy impl that is called from all sorts of locations, the most concerning of which is the
    // abstractnetworkjob
    virtual bool stillValid(QNetworkReply *reply) = 0;

    virtual void persist() = 0;

    /** Invalidates token used to authorize requests, it will no longer be used.
     *
     * For http auth, this would be the session cookie.
     *
     * Note that sensitive data (like the password used to acquire the
     * session cookie) may be retained. See forgetSensitiveData().
     *
     * ready() must return false afterwards.
     */
    virtual void invalidateToken() = 0;

    /** Clears out all sensitive data; used for fully signing out users.
     *
     * This should always imply invalidateToken() but may go beyond it.
     *
     * For http auth, this would clear the session cookie and password.
     */
    virtual void forgetSensitiveData() = 0;

Q_SIGNALS:
    /** Emitted when fetchFromKeychain() is done.
     *
     * Note that ready() can be true or false, depending on whether there was useful
     * data in the keychain.
     */
    // TODO: rename
    void fetched();

    void authenticationStarted();
    void authenticationFailed();

    /*
     * Request to log out.
     * The connected account should be marked as logged out
     * and no automatic tries to connect should be made.
     */
    void requestLogout();

protected:
    // the account should be the parent of the creds, so it should not go out of scope while we are using this.
    // however, those may be famous last words depending on how the tear
    Account *_account;

    // todo: DC-112 I don't understand why this is needed but will try to figure it out
    bool _wasEverFetched;
};

} // namespace OCC
