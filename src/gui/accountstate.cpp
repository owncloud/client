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

#include "accountstate.h"
#include "account.h"
#include "accountmanager.h"
#include "application.h"
#include "configfile.h"
#include "fetchserversettings.h"

#include "libsync/creds/abstractcredentials.h"
#include "libsync/creds/httpcredentials.h"

#include "gui/networkinformation.h"
#include "gui/quotainfo.h"
#include "gui/settingsdialog.h"
#include "gui/tlserrordialog.h"

#include "socketapi/socketapi.h"
#include "theme.h"

#include <QFontMetrics>
#include <QRandomGenerator>
#include <QSettings>
#include <QTimer>

using namespace std::chrono;
using namespace std::chrono_literals;

namespace {

inline const QLatin1String userExplicitlySignedOutC()
{
    return QLatin1String("userExplicitlySignedOut");
}
auto supportsSpacesC()
{
    return QLatin1String("supportsSpaces");
}
} // anonymous namespace

namespace OCC {

Q_LOGGING_CATEGORY(lcAccountState, "gui.account.state", QtInfoMsg)

// Returns the dialog when one is shown, so callers can attach to signals. If no dialog is shown
// (because there is one already, or the new URL matches the current URL), a nullptr is returned.
UpdateUrlDialog *AccountState::updateUrlDialog(const QUrl &newUrl)
{
    // guard to prevent multiple dialogs
    if (_updateUrlDialog) {
        return nullptr;
    }

    _updateUrlDialog = UpdateUrlDialog::fromAccount(_account, newUrl, ocApp()->gui()->settingsDialog());

    connect(_updateUrlDialog, &UpdateUrlDialog::accepted, this, [=]() {
        _account->setUrl(newUrl);
        Q_EMIT _account->wantsAccountSaved(_account.data());
        Q_EMIT urlUpdated();
    });

    ownCloudGui::raise();
    _updateUrlDialog->open();

    return _updateUrlDialog;
}

AccountState::AccountState(AccountPtr account)
    : QObject()
    , _account(account)
    , _queueGuard(_account->jobQueue())
    , _state(AccountState::Disconnected)
    , _connectionStatus(ConnectionValidator::Undefined)
    , _waitingForNewCredentials(false)
    , _maintenanceToConnectedDelay(1min + minutes(QRandomGenerator::global()->generate() % 4)) // 1-5min delay
{
    qRegisterMetaType<AccountState *>("AccountState*");

    connect(account.data(), &Account::invalidCredentials,
        this, &AccountState::slotInvalidCredentials);
    connect(account.data(), &Account::credentialsFetched,
        this, &AccountState::slotCredentialsFetched);
    connect(account.data(), &Account::credentialsAsked,
        this, &AccountState::slotCredentialsAsked);
    connect(account.data(), &Account::unknownConnectionState,
        this, [this] {
            checkConnectivity(true);
        });
    connect(account.data(), &Account::requestUrlUpdate, this, &AccountState::updateUrlDialog);
    connect(this, &AccountState::urlUpdated, this, [this] {
        checkConnectivity(false);
    });
    connect(account.data(), &Account::requestUrlUpdate, this, &AccountState::updateUrlDialog, Qt::QueuedConnection);
    connect(
        this, &AccountState::urlUpdated, this, [this] {
            checkConnectivity(false);
        },
        Qt::QueuedConnection);

    connect(NetworkInformation::instance(), &NetworkInformation::reachabilityChanged, this, [this](NetworkInformation::Reachability reachability) {
        switch (reachability) {
        case NetworkInformation::Reachability::Online:
            [[fallthrough]];
        case NetworkInformation::Reachability::Site:
            [[fallthrough]];
        case NetworkInformation::Reachability::Unknown:
            // the connection might not yet be established
            QTimer::singleShot(0, this, [this] { checkConnectivity(false); });
            break;
        case NetworkInformation::Reachability::Disconnected:
            // explicitly set disconnected, this way a successful checkConnectivity call above will trigger a local discover
            if (state() != State::SignedOut) {
                setState(State::Disconnected);
            }
            [[fallthrough]];
        case NetworkInformation::Reachability::Local:
            break;
        }
    });

    connect(NetworkInformation::instance(), &NetworkInformation::isMeteredChanged, this, [this](bool isMetered) {
        if (ConfigFile().pauseSyncWhenMetered()) {
            if (state() == State::Connected && isMetered) {
                qCInfo(lcAccountState) << "Network switched to a metered connection, setting account state to PausedDueToMetered";
                setState(State::Connecting);
            } else if (state() == State::Connecting && !isMetered) {
                qCInfo(lcAccountState) << "Network switched to a NON-metered connection, setting account state to Connected";
                setState(State::Connected);
            }
        }
    });

    // todo: #12
    connect(NetworkInformation::instance(), &NetworkInformation::isBehindCaptivePortalChanged, this, [this](bool onoff) {
        if (onoff) {
            // Block jobs from starting: they will fail because of the captive portal.
            // Note: this includes the `Drives` jobs started periodically by the `SpacesManager`.
            _queueGuard.block();
        } else {
            // Empty the jobs queue before unblocking it. The client might have been behind a captive
            // portal for hours, so a whole bunch of jobs might have queued up. If we wouldn't
            // clear the queue, unleashing all those jobs might look like a DoS attack. Most of them
            // are also not very useful anymore (e.g. `Drives` jobs), and the important ones (PUT jobs)
            // will be rescheduled by a directory scan.
            _account->jobQueue()->clear();
            _queueGuard.unblock();
        }

        // A direct connect is not possible, because then the state parameter of `isBehindCaptivePortalChanged`
        // would become the `verifyServerState` argument to `checkConnectivity`.
        // The call is also made for when we "go behind" a captive portal. That ensures that not
        // only the status is set to `Connecting`, but also makes the UI show that syncing is paused.
        // todo: #11, #12
        QTimer::singleShot(0, this, [this] { checkConnectivity(false); });
    });
    if (NetworkInformation::instance()->isBehindCaptivePortal()) {
        _queueGuard.block();
    }

    // as a fallback and to recover after server issues we also poll
    auto timer = new QTimer(this);
    timer->setInterval(ConnectionValidator::DefaultCallingInterval);
    connect(timer, &QTimer::timeout, this, [this] { checkConnectivity(false); });
    timer->start();

    connect(account->credentials(), &AbstractCredentials::requestLogout, this, [this] {
        setState(State::SignedOut);
    });

    if (FolderMan::instance()) {
        FolderMan::instance()->socketApi()->registerAccount(account);
    }

    connect(account.data(), &Account::appProviderErrorOccured, this, [](const QString &error) {
        QMessageBox *msgBox = new QMessageBox(QMessageBox::Information, Theme::instance()->appNameGUI(), error, {}, ocApp()->gui()->settingsDialog());
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        ownCloudGui::raise();
        msgBox->open();
    });
}

AccountState::~AccountState() { }

std::unique_ptr<AccountState> AccountState::loadFromSettings(AccountPtr account, const QSettings &settings)
{
    auto accountState = std::unique_ptr<AccountState>(new AccountState(account));
    const bool userExplicitlySignedOut = settings.value(userExplicitlySignedOutC(), false).toBool();
    if (userExplicitlySignedOut) {
        // see writeToSettings below
        accountState->setState(SignedOut);
    }
    accountState->_supportsSpaces = settings.value(supportsSpacesC(), false).toBool();
    return accountState;
}

std::unique_ptr<AccountState> AccountState::fromNewAccount(AccountPtr account)
{
    return std::unique_ptr<AccountState>(new AccountState(account));
}

void AccountState::writeToSettings(QSettings &settings) const
{
    // The SignedOut state is the only state where the client should *not* ask for credentials, nor
    // try to connect to the server. All other states should transition to Connected by either
    // (re-)trying to make a connection, or by authenticating (AskCredentials). So we save the
    // SignedOut state to indicate that the client should not try to re-connect the next time it
    // is started.
    settings.setValue(userExplicitlySignedOutC(), _state == SignedOut);
    settings.setValue(supportsSpacesC(), _supportsSpaces);
}

AccountPtr AccountState::account() const
{
    return _account;
}

AccountState::ConnectionStatus AccountState::connectionStatus() const
{
    return _connectionStatus;
}

QStringList AccountState::connectionErrors() const
{
    return _connectionErrors;
}

AccountState::State AccountState::state() const
{
    return _state;
}

void AccountState::setState(State state)
{
    const State oldState = _state;
    if (_state != state) {
        qCInfo(lcAccountState) << "AccountState state change: " << _state << "->" << state;
        _state = state;

        if (_state == SignedOut) {
            _connectionStatus = ConnectionValidator::Undefined;
            _connectionErrors.clear();
        } else if (oldState == SignedOut && _state == Disconnected) {
            // If we stop being voluntarily signed-out, try to connect and
            // auth right now!
            checkConnectivity();
        } else if (_state == ServiceUnavailable) {
            // Check if we are actually down for maintenance.
            // To do this we must clear the connection validator that just
            // produced the 503. It's finished anyway and will delete itself.
            _connectionValidator->deleteLater();
            _connectionValidator.clear();
            checkConnectivity();
        } else if (_state == Connected) {
            if ((NetworkInformation::instance()->isMetered() && ConfigFile().pauseSyncWhenMetered())
                || NetworkInformation::instance()->isBehindCaptivePortal()) {
                _state = Connecting;
            }
        }
    }

    // might not have changed but the underlying _connectionErrors might have
    if (_state == Connected) {
        QTimer::singleShot(0, this, [this, oldState] {
            // ensure the connection validator is done
            _queueGuard.unblock();
            // update capabilites and fetch relevant settings
            _fetchCapabilitiesJob = new FetchServerSettingsJob(account(), this);
            connect(_fetchCapabilitiesJob.get(), &FetchServerSettingsJob::finishedSignal, this, [oldState, this] {
                if (oldState == Connected || _state == Connected) {
                    _fetchCapabilitiesJob.clear();
                    Q_EMIT isConnectedChanged();
                    Q_EMIT supportsSpacesChanged();
                }
            });
            _fetchCapabilitiesJob->start();
        });
    }

    if (oldState != _state) {
        Q_EMIT stateChanged(_state);
    }
}

bool AccountState::isSignedOut() const
{
    return _state == SignedOut;
}

void AccountState::signOutByUi()
{
    account()->credentials()->forgetSensitiveData();
    account()->clearCookieJar();
    setState(SignedOut);
    // persist that we are signed out
    Q_EMIT account()->wantsAccountSaved(account().data());
}

void AccountState::freshConnectionAttempt()
{
    if (isConnected())
        setState(Disconnected);
    checkConnectivity();
}

void AccountState::signIn()
{
    if (_state == SignedOut) {
        _waitingForNewCredentials = false;
        setState(Disconnected);
        // persist that we are no longer signed out
        Q_EMIT account()->wantsAccountSaved(account().data());
    }
}

bool AccountState::isConnected() const
{
    return _state == Connected;
}

void AccountState::tagLastSuccessfullETagRequest(const QDateTime &tp)
{
    _timeOfLastETagCheck = tp;
}

void AccountState::checkConnectivity(bool blockJobs)
{
    if (isSignedOut() || _waitingForNewCredentials) {
        return;
    }
    qCInfo(lcAccountState) << "checkConnectivity blocking:" << blockJobs << account()->displayNameWithHost();
    if (_state != Connected) {
        setState(Connecting);
    }
    if (_tlsDialog) {
        qCDebug(lcAccountState) << "Skip checkConnectivity, waiting for tls dialog";
        return;
    }


    if (_connectionValidator && blockJobs && !_queueGuard.queue()->isBlocked()) {
        // abort already running non blocking validator
        _connectionValidator->deleteLater();
        _connectionValidator.clear();
    }
    if (_connectionValidator) {
        qCWarning(lcAccountState) << "ConnectionValidator already running, ignoring" << account()->displayNameWithHost()
                                  << "Queue is blocked:" << _queueGuard.queue()->isBlocked();
        return;
    }

    // If we never fetched credentials, do that now - otherwise connection attempts
    // make little sense.
    if (!account()->credentials()->wasFetched()) {
        _waitingForNewCredentials = true;
        account()->credentials()->fetchFromKeychain();
    }
    if (account()->hasCapabilities()) {
        // IF the account is connected the connection check can be skipped
        // if the last successful etag check job is not so long ago.
        // TODO: https://github.com/owncloud/client/issues/10935
        const auto pta = account()->capabilities().remotePollInterval();
        const auto polltime = duration_cast<seconds>(ConfigFile().remotePollInterval(pta));
        const auto elapsed = _timeOfLastETagCheck.secsTo(QDateTime::currentDateTimeUtc());
        if (!blockJobs && isConnected() && _timeOfLastETagCheck.isValid()
            && elapsed <= polltime.count()) {
            qCDebug(lcAccountState) << account()->displayNameWithHost() << "The last ETag check succeeded within the last " << polltime.count() << "s ("
                                    << elapsed << "s). No connection check needed!";
            return;
        }
    }

    if (blockJobs) {
        _queueGuard.block();
    }
    _connectionValidator = new ConnectionValidator(account());
    connect(_connectionValidator, &ConnectionValidator::connectionResult,
        this, &AccountState::slotConnectionValidatorResult);

    connect(_connectionValidator, &ConnectionValidator::sslErrors, this, [blockJobs, this](const QList<QSslError> &errors) {
        if (NetworkInformation::instance()->isBehindCaptivePortal()) {
            return;
        }
        if (!_tlsDialog) {
            // ignore errors for already accepted certificates
            auto filteredErrors = _account->accessManager()->filterSslErrors(errors);
            if (!filteredErrors.isEmpty()) {
                _tlsDialog = new TlsErrorDialog(filteredErrors, _account->url().host(), ocApp()->gui()->settingsDialog());
                _tlsDialog->setAttribute(Qt::WA_DeleteOnClose);
                QSet<QSslCertificate> certs;
                certs.reserve(filteredErrors.size());
                for (const auto &error : std::as_const(filteredErrors)) {
                    certs << error.certificate();
                }
                connect(_tlsDialog, &TlsErrorDialog::accepted, _tlsDialog, [certs, blockJobs, this]() {
                    _account->addApprovedCerts(certs);
                    _tlsDialog.clear();
                    // force a new _connectionValidator
                    if (_connectionValidator) {
                        _connectionValidator->deleteLater();
                        _connectionValidator.clear();
                    }
                    checkConnectivity(blockJobs);
                });
                connect(_tlsDialog, &TlsErrorDialog::rejected, this, [certs, this]() {
                    setState(SignedOut);
                });

                ownCloudGui::raise();
                _tlsDialog->open();
            }
        }
    });
    ConnectionValidator::ValidationMode mode = ConnectionValidator::ValidationMode::ValidateAuthAndUpdate;
    if (isConnected()) {
        // Use a small authed propfind as a minimal ping when we're
        // already connected.
        if (blockJobs) {
            _connectionValidator->setClearCookies(true);
            mode = ConnectionValidator::ValidationMode::ValidateAuth;
        } else {
            mode = ConnectionValidator::ValidationMode::ValidateAuthAndUpdate;
        }
    } else {
        // Check the server and then the auth.
        if (_waitingForNewCredentials) {
            mode = ConnectionValidator::ValidationMode::ValidateServer;
        } else {
            _connectionValidator->setClearCookies(true);
            mode = ConnectionValidator::ValidationMode::ValidateAuthAndUpdate;
        }
    }
    _connectionValidator->checkServer(mode);
}

void AccountState::slotConnectionValidatorResult(ConnectionValidator::Status status, const QStringList &errors)
{
    if (isSignedOut()) {
        qCWarning(lcAccountState) << "Signed out, ignoring" << status << _account->url().toString();
        return;
    }


    if (status == ConnectionValidator::Connected && !_account->hasCapabilities()) {
        // this code should only be needed when upgrading from a < 3.0 release where capabilities where not cached
        // The last check was _waitingForNewCredentials = true so we only checked ValidateServer
        // now check again and fetch capabilities
        _connectionValidator->deleteLater();
        _connectionValidator.clear();
        checkConnectivity();
        return;
    }

    // Come online gradually from 503 or maintenance mode
    if (status == ConnectionValidator::Connected
        && (_connectionStatus == ConnectionValidator::ServiceUnavailable
               || _connectionStatus == ConnectionValidator::MaintenanceMode)) {
        if (!_timeSinceMaintenanceOver.isValid()) {
            qCInfo(lcAccountState) << "AccountState reconnection: delaying for"
                                   << _maintenanceToConnectedDelay.count() << "ms";
            _timeSinceMaintenanceOver.start();
            QTimer::singleShot(_maintenanceToConnectedDelay + 100ms, this, [this] { AccountState::checkConnectivity(false); });
            return;
        } else if (_timeSinceMaintenanceOver.elapsed() < _maintenanceToConnectedDelay.count()) {
            qCInfo(lcAccountState) << "AccountState reconnection: only"
                                   << _timeSinceMaintenanceOver.elapsed() << "ms have passed";
            return;
        }
    }

    if (_connectionStatus != status) {
        qCInfo(lcAccountState) << "AccountState connection status change: "
                               << _connectionStatus << "->"
                               << status;
        _connectionStatus = status;
    }
    _connectionErrors = errors;

    switch (status) {
    case ConnectionValidator::Connected:
        setState(Connected);
        break;
    case ConnectionValidator::Undefined:
        [[fallthrough]];
    case ConnectionValidator::NotConfigured:
        setState(Disconnected);
        break;
    case ConnectionValidator::ClientUnsupported:
        [[fallthrough]];
    case ConnectionValidator::ServerVersionMismatch:
        setState(ConfigurationError);
        break;
    case ConnectionValidator::StatusNotFound:
        // This can happen either because the server does not exist
        // or because we are having network issues. The latter one is
        // much more likely, so keep trying to connect.
        setState(NetworkError);
        break;
    case ConnectionValidator::CredentialsWrong:
        [[fallthrough]];
    case ConnectionValidator::CredentialsNotReady:
        slotInvalidCredentials();
        break;
    case ConnectionValidator::SslError:
        // handled with the tlsDialog
        break;
    case ConnectionValidator::ServiceUnavailable:
        _timeSinceMaintenanceOver.invalidate();
        setState(ServiceUnavailable);
        break;
    case ConnectionValidator::MaintenanceMode:
        _timeSinceMaintenanceOver.invalidate();
        setState(MaintenanceMode);
        break;
    case ConnectionValidator::Timeout:
        setState(NetworkError);
        break;
    case ConnectionValidator::CaptivePortal:
        setState(Connecting);
        break;
    }
}

void AccountState::slotInvalidCredentials()
{
    if (!_waitingForNewCredentials) {
        qCInfo(lcAccountState) << "Invalid credentials for" << _account->url().toString();

        _waitingForNewCredentials = true;
        if (account()->credentials()->ready()) {
            account()->credentials()->invalidateToken();
        }
        if (auto creds = qobject_cast<HttpCredentials *>(account()->credentials())) {
            qCInfo(lcAccountState) << "refreshing oauth";
            if (creds->refreshAccessToken()) {
                return;
            }
            qCInfo(lcAccountState) << "refreshing oauth failed";
        }
        qCInfo(lcAccountState) << "asking user";
        account()->credentials()->askFromUser();
        setState(AskingCredentials);
    }
}

void AccountState::slotCredentialsFetched()
{
    // Make a connection attempt, no matter whether the credentials are
    // ready or not - we want to check whether we can get an SSL connection
    // going before bothering the user for a password.
    qCInfo(lcAccountState) << "Fetched credentials for" << _account->url().toString()
                           << "attempting to connect";
    _waitingForNewCredentials = false;
    checkConnectivity();
}

void AccountState::slotCredentialsAsked()
{
    qCInfo(lcAccountState) << "Credentials asked for" << _account->url().toString() << "are they ready?" << _account->credentials()->ready();

    _waitingForNewCredentials = false;

    if (!_account->credentials()->ready()) {
        // User canceled the connection or did not give a password
        setState(SignedOut);
        return;
    }

    if (_connectionValidator) {
        // When new credentials become available we always want to restart the
        // connection validation, even if it's currently running.
        _connectionValidator->deleteLater();
        _connectionValidator.clear();
    }

    checkConnectivity();
}

Account *AccountState::accountForQml() const
{
    return _account.data();
}

std::unique_ptr<QSettings> AccountState::settings()
{
    auto s = ConfigFile::settingsWithGroup(QStringLiteral("Accounts"));
    s->beginGroup(_account->id());
    return s;
}

bool AccountState::supportsSpaces() const
{
    return _supportsSpaces && _account->hasCapabilities() && _account->capabilities().spacesSupport().enabled;
}

QuotaInfo *AccountState::quotaInfo()
{
    // QuotaInfo should not be used with spaces
    Q_ASSERT(!supportsSpaces());
    if (!_quotaInfo) {
        _quotaInfo = new QuotaInfo(this);
    }
    return _quotaInfo;
}

bool AccountState::isSettingUp() const
{
    return _settingUp;
}

// Refactoring todo: We currently call this from owncloudgui to set the val true then false at a later point
// actual consumer is in the AccountSettings gui - it shows a spinny from the time it's set to
// true to when it switches to false. IMO this does NOT belong in the AccountState if it can't
// manage the value itself. It needs to go elsewhere, ideally the owncloudgui can just call
// into the account settings directly as any good controller would
void AccountState::setSettingUp(bool settingUp)
{
    if (_settingUp != settingUp) {
        _settingUp = settingUp;
        // for goodness sake, just send the new value
        Q_EMIT isSettingUpChanged();
    }
}
bool AccountState::readyForSync() const
{
    return !_fetchCapabilitiesJob && isConnected();
}

} // namespace OCC
