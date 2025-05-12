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

#include "gui/networkinformation.h"
#include "gui/owncloudguilib.h"

#include "connectionvalidator.h"
#include "creds/abstractcredentials.h"
#include "jobqueue.h"

#include "account.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QPointer>
#include <memory>
#include <qqmlintegration.h>


class QDialog;
class QMessageBox;
class QSettings;

namespace OCC {

class QuotaInfo;
class TlsErrorDialog;
class FetchServerSettingsJob;

/**
 * @brief Extra info about an ownCloud server account.
 * @ingroup gui
 */
class OWNCLOUDGUI_EXPORT AccountState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Account *account READ accountForQml CONSTANT)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY isConnectedChanged)
    // todo: #16
    Q_PROPERTY(bool supportsSpaces READ supportsSpaces NOTIFY supportsSpacesChanged)
    Q_PROPERTY(AccountState::State state READ state NOTIFY stateChanged)
    QML_ELEMENT
    QML_UNCREATABLE("Only created by AccountManager")

public:
    enum State {
        /// Not even attempting to connect, most likely because the
        /// user explicitly signed out or cancelled a credential dialog.
        SignedOut,

        /// Account would like to be connected but hasn't heard back yet.
        Disconnected,

        /// The account is successfully talking to the server.
        Connected,

        /// There's a temporary problem with talking to the server,
        /// don't bother the user too much and try again.
        ServiceUnavailable,

        /// Similar to ServiceUnavailable, but we know the server is down
        /// for maintenance
        MaintenanceMode,

        /// Could not communicate with the server for some reason.
        /// We assume this may resolve itself over time and will try
        /// again automatically.
        NetworkError,

        /// Server configuration error. (For example: unsupported version)
        ConfigurationError,

        /// We are currently asking the user for credentials
        AskingCredentials,

        Connecting
    };
    Q_ENUM(State)

    /// The actual current connectivity status.
    typedef ConnectionValidator::Status ConnectionStatus;

    ~AccountState() override;

    /** Creates an account state from settings and an Account object.
     *
     * Use from AccountManager with a prepared QSettings object only.
     */
    static std::unique_ptr<AccountState> loadFromSettings(AccountPtr account, const QSettings &settings);

    static std::unique_ptr<AccountState> fromNewAccount(AccountPtr account);

    /** Writes account state information to settings.
     *
     * It does not write the Account data.
     */
    void writeToSettings(QSettings &settings) const;

    AccountPtr account() const;

    ConnectionStatus connectionStatus() const;
    QStringList connectionErrors() const;

    State state() const;

    bool isSignedOut() const;

    [[nodiscard]] bool readyForSync() const;

    /** A user-triggered sign out which disconnects, stops syncs
     * for the account and forgets the password. */
    void signOutByUi();

    /** Tries to connect from scratch.
     *
     * Does nothing for signed out accounts.
     * Connected accounts will be disconnected and try anew.
     * Disconnected accounts will go to checkConnectivity().
     *
     * Useful for when network settings (proxy) change.
     */
    void freshConnectionAttempt();

    /// Move from SignedOut state to Disconnected (attempting to connect)
    void signIn();

    bool isConnected() const;

    // weather the account was created after spaces where implemented
    bool supportsSpaces() const;

    QuotaInfo *quotaInfo();

    /** Returns a new settings object for this account, already in the right groups. */
    std::unique_ptr<QSettings> settings();

    /** Mark the timestamp when the last successful ETag check happened for
     *  this account.
     *  The checkConnectivity() method uses the timestamp to save a call to
     *  the server to validate the connection if the last successful etag job
     *  was not so long ago.
     */
    void tagLastSuccessfulETagRequest(const QDateTime &tp);

    /***
     * The account is set up for the first time, this may take some time
     */
    bool isSettingUp() const;
    void setSettingUp(bool settingUp);

    /// Triggers a ping to the server to update state, connection status and errors.
    /// blockJobs determines if we block the job queue while the connection is checked
    void checkConnectivity(bool blockJobs = false);

private:
    /// Use the account as parent
    explicit AccountState(AccountPtr account);

    void setState(State state);

Q_SIGNALS:
    void stateChanged(State state);
    void isConnectedChanged();
    void isSettingUpChanged();
    void supportsSpacesChanged();

protected Q_SLOTS:
    void slotConnectionValidatorResult(ConnectionValidator::Status status, const QStringList &errors);
    void slotInvalidCredentials();
    void slotCredentialsFetched();
    void slotCredentialsAsked();

private:
    Account *accountForQml() const;
    AccountPtr _account;
    JobQueueGuard _queueGuard;
    State _state;
    ConnectionStatus _connectionStatus;
    QStringList _connectionErrors;
    bool _waitingForNewCredentials;
    QDateTime _timeOfLastETagCheck;
    bool _supportsSpaces = true;
    bool _settingUp = false;

    ConnectionValidator *_connectionValidator;
    void resetConnectionValidator();

    void connectAccount();
    void connectNetworkInformation();

    void onNetworkReachabilityChanged(NetworkInformation::Reachability reachability);
    void onNetworkMeteredChanged(bool isMetered);
    void onBehindCaptivePortalChanged(bool isCaptive);

    // this doesn't completely thrill me because it has dependencies on the rest of the gui but for now, it's
    // ok and making it private is pretty sweet, too.
    void confirmUrlUpdate(const QUrl &newUrl);

    // handles ssl errors from the ConnectionValidator
    // if there are ssl errors it will pop a dialog asking user to accept new certificate.
    // if that is accepted, we update the certificate on the account, then rerun checkConnection using jobsWereBlocked,
    // which is what was in play during the connection check that produced the error(s)
    void handleSslConnectionErrors(const QList<QSslError> &errors, bool jobsWereBlocked);

    /**
     * Starts counting when the server starts being back up after 503 or
     * maintenance mode. The account will only become connected once this
     * timer exceeds the _maintenanceToConnectedDelay value.
     */
    QElapsedTimer _timeSinceMaintenanceOver;

    /**
     * Milliseconds for which to delay reconnection after 503/maintenance.
     */
    std::chrono::milliseconds _maintenanceToConnectedDelay;

    QuotaInfo *_quotaInfo = nullptr;

    QPointer<FetchServerSettingsJob> _fetchCapabilitiesJob;
};
}

Q_DECLARE_METATYPE(OCC::AccountState *)
Q_DECLARE_METATYPE(OCC::AccountStatePtr)
