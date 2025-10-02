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

#include "oauth.h"

#include "accessmanager.h"
#include "creds/credentialssupport.h"
#include "gui/networkadapters/userinfoadapter.h"
#include "libsync/creds/credentialmanager.h"
#include "networkjobs/checkserverjobfactory.h"
#include "oauthhtmlpage.h"
#include "theme.h"

#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRandomGenerator>

using namespace std::chrono;
using namespace std::chrono_literals;

using namespace OCC;

// todo: #24 - this is such a mess. it needs to parted out into clear classes (each in their own files), every impl needs to be reviewed.
// I think there is some duplication here as well - just sort it out as much as possible.
Q_LOGGING_CATEGORY(lcOauth, "sync.credentials.oauth", QtInfoMsg)

namespace {

const QString wellKnownPathC = QStringLiteral("/.well-known/openid-configuration");

auto defaultOauthPromptValue()
{
    static const auto promptValue = [] {
        OAuth::PromptValuesSupportedFlags out = OAuth::PromptValuesSupported::none;
        // convert the legacy openIdConnectPrompt() to QFlags
        for (const auto &x : Theme::instance()->openIdConnectPrompt().split(QLatin1Char(' '))) {
            out |= Utility::stringToEnum<OAuth::PromptValuesSupported>(x);
        }
        return out;
    }();
    return promptValue;
}

seconds defaultTimeout()
{
    // as the OAuth process can be interactive we don't want 5min of inactivity
    return qMin(30s, OCC::AbstractNetworkJob::httpTimeout);
}

int defaultTimeoutMs()
{
    return static_cast<int>(duration_cast<milliseconds>(defaultTimeout()).count());
}

QVariant getRequiredField(const QVariantMap &json, const QString &s, QString *error)
{
    const auto out = json.constFind(s);
    if (out == json.constEnd()) {
        error->append(QStringLiteral("\tError: Missing field %1\n").arg(s));
        return {};
    }
    return *out;
}
}

OAuth::OAuth(const QUrl &serverUrl, const QString &davUser, QNetworkAccessManager *networkAccessManager, QObject *parent)
    : QObject(parent)
    , _serverUrl(serverUrl)
    , _davUser(davUser)
    , _networkAccessManager(networkAccessManager)
    , _clientId(Theme::instance()->oauthClientId())
    , _clientSecret(Theme::instance()->oauthClientSecret())
    , _redirectUrl(QString("http://localhost"))
    , _supportedPromptValues(defaultOauthPromptValue())
{
}

OAuth::~OAuth() = default;

void OAuth::startAuthentication()
{
    qCDebug(lcOauth) << "starting authentication";

    // Listen on the socket to get a port which will be used in the redirect_uri

    QList<quint16> ports = Theme::instance()->oauthPorts();
    for (const auto port : std::as_const(ports)) {
        if (_server.listen(QHostAddress::LocalHost, port)) {
            break;
        }
        qCDebug(lcOauth) << "Creating local server Port:" << port << "failed. Error:" << _server.errorString();
    }
    if (!_server.isListening()) {
        qCDebug(lcOauth) << "server is not listening";
        Q_EMIT result(Error, {});
        return;
    }

    _pkceCodeVerifier = generateRandomString(24);
    OC_ASSERT(_pkceCodeVerifier.size() == 128)
    _state = generateRandomString(8);

    connect(this, &OAuth::fetchWellKnownFinished, this, &OAuth::authorisationLinkChanged);

    fetchWellKnown();

    QObject::connect(&_server, &QTcpServer::newConnection, this, &OAuth::handleTcpConnection);
}

void OAuth::httpReplyAndClose(QPointer<QTcpSocket> socket, bool success, const QString &code, const QString &title, const QString &body, const QStringList &additionalHeader)
{
    if (!socket) {
        return; // socket can have been deleted if the browser was closed
    }

    const QByteArray content = OAuthHtmlPage::buildPage(success, title, body.isEmpty() ? title : body).toUtf8();
    QString header = QStringLiteral("HTTP/1.1 %1\r\n"
                                    "Content-Type: text/html; charset=utf-8\r\n"
                                    "Connection: close\r\n"
                                    "Content-Length: %2\r\n")
                         .arg(code, QString::number(content.length()));

    if (!additionalHeader.isEmpty()) {
        const QString nl = QStringLiteral("\r\n");
        header += additionalHeader.join(nl) + nl;
    }

    const QByteArray msg = header.toUtf8() + QByteArrayLiteral("\r\n") + content;

    qCDebug(lcOauth) << "replying with HTTP response and closing socket:" << msg;

    socket->write(msg);
    socket->disconnectFromHost();
    // this is super important as we may delete the oauth/server before the socket has completed its work
    // let the deleteLater on final disconnect take care of the cleanup
    socket->setParent(nullptr);
}

void OAuth::finalize(const QString &accessToken, const QString &refreshToken, const QUrl &messageUrl)
{
    const QString brand = Theme::instance()->appNameGUI();
    const QString loginSuccessfulTitle = tr("Successfully signed in");
    const QString loginSuccessfulHtml = tr("Now, explore %1 on desktop.").arg(brand);

    if (messageUrl.isValid()) {
        httpReplyAndClose(_socket, true, QStringLiteral("303 See Other"), loginSuccessfulTitle, loginSuccessfulHtml,
            {QStringLiteral("Location: %1").arg(QString::fromUtf8(messageUrl.toEncoded()))});
    } else {
        httpReplyAndClose(_socket, true, QStringLiteral("200 OK"), loginSuccessfulTitle, loginSuccessfulHtml);
    }
    Q_EMIT result(LoggedIn, accessToken, refreshToken);
}

// the tcp server handling has to be done async while we wait for a suitable socket with which to communiciate with the
// authentication web page. The async handling for tcp includes this function + the handleSocketReadyRead
void OAuth::handleTcpConnection()
{
    while (_server.hasPendingConnections()) {
        auto socket = _server.nextPendingConnection();

        qCDebug(lcOauth) << "accepted client connection from" << socket->peerAddress();

        QObject::connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);

        QObject::connect(socket, &QIODevice::readyRead, this, &OAuth::handleSocketReadyRead);
    }
}

void OAuth::handleSocketReadyRead()
{
    // we already have the socket we want to use to complete the auth communications so ignore any other incoming
    // sockets
    if (!_server.isListening())
        return;

    QTcpSocket *connected = qobject_cast<QTcpSocket *>(sender());
    if (!connected) {
        // something is really wrong
        return;
    }
    QPointer<QTcpSocket> candidateSocket(connected);

    Q_ASSERT(connected->state() == QAbstractSocket::ConnectedState);

    const QByteArray peek = candidateSocket->peek(qMin(connected->bytesAvailable(), 4000LL)); // The code should always be within the first 4K

    // wait until we find a \n
    if (!peek.contains('\n')) {
        return;
    }

    qCDebug(lcOauth) << "Server provided:" << peek;

    const auto getPrefix = QByteArrayLiteral("GET /?");
    if (!peek.startsWith(getPrefix)) {
        httpReplyAndClose(candidateSocket, false, QStringLiteral("404 Not Found"), QStringLiteral("404 Not Found"));
        return;
    }
    const auto endOfUrl = peek.indexOf(' ', getPrefix.length());

    _queryArgs = QUrlQuery(QUrl::fromPercentEncoding(peek.mid(getPrefix.length(), endOfUrl - getPrefix.length())));

    if (_queryArgs.queryItemValue(QStringLiteral("state")).toUtf8() != _state) {
        httpReplyAndClose(candidateSocket, false, QStringLiteral("400 Bad Request"), QStringLiteral("400 Bad Request"));
        return;
    }

    // basic checks passed, we now want to commit to this socket all other pending connections will effectively be
    // ignored from here out. To ensure we get as few stray connections as possible, just close the server
    _socket = candidateSocket;

    // server port cannot be queried any more after server has been closed, which we want to do as early as possible in the processing chain
    // therefore we have to store it beforehand
    _serverPort = _server.serverPort();
    _server.close();

    getTokens();
}

// todo: #24 - this can be converted to an adapter
void OAuth::getTokens()
{
    if(_tokenEndpoint.isEmpty()) {
        Q_EMIT result(Error);
        return;
    }

    auto postTokenReply = postTokenRequest({
        {QStringLiteral("grant_type"), QStringLiteral("authorization_code")},
        {QStringLiteral("code"), _queryArgs.queryItemValue(QStringLiteral("code"))},
        {QStringLiteral("redirect_uri"), QStringLiteral("%1:%2").arg(_redirectUrl, QString::number(_serverPort))},
        {QStringLiteral("code_verifier"), QString::fromUtf8(_pkceCodeVerifier)},
    });

    connect(postTokenReply, &QNetworkReply::finished, this, [postTokenReply, this] {
        const auto jsonData = postTokenReply->readAll();
        QJsonParseError jsonParseError;
        const auto data = QJsonDocument::fromJson(jsonData, &jsonParseError).object().toVariantMap();
        QString fieldsError;
        _accessToken = getRequiredField(data, QStringLiteral("access_token"), &fieldsError).toString();
        _refreshToken = getRequiredField(data, QStringLiteral("refresh_token"), &fieldsError).toString();
        const QString tokenType = getRequiredField(data, QStringLiteral("token_type"), &fieldsError).toString().toLower();
        _messageUrl = QUrl::fromEncoded(data[QStringLiteral("message_url")].toByteArray());

        if (postTokenReply->error() != QNetworkReply::NoError || jsonParseError.error != QJsonParseError::NoError || !fieldsError.isEmpty()
            || tokenType != QLatin1String("bearer")) {
            // do we have error message suitable for users?
            QString errorReason = data[QStringLiteral("error_description")].toString();
            if (errorReason.isEmpty()) {
                // fall back to technical error
                errorReason = data[QStringLiteral("error")].toString();
            }
            if (!errorReason.isEmpty()) {
                errorReason = tr("Error returned from the server: <em>%1</em>").arg(errorReason.toHtmlEscaped());
            } else if (postTokenReply->error() != QNetworkReply::NoError) {
                errorReason = tr("There was an error accessing the 'token' endpoint: <br><em>%1</em>").arg(postTokenReply->errorString().toHtmlEscaped());
            } else if (jsonParseError.error != QJsonParseError::NoError) {
                errorReason = tr("Could not parse the JSON returned from the server: <br><em>%1</em>").arg(jsonParseError.errorString());
            } else if (tokenType != QStringLiteral("bearer")) {
                errorReason = tr("Unsupported token type: %1").arg(tokenType);
            } else if (!fieldsError.isEmpty()) {
                errorReason = tr("The reply from the server did not contain all expected fields\n:%1").arg(fieldsError);
            } else {
                errorReason = tr("Unknown Error");
            }
            qCWarning(lcOauth) << "Error when getting the accessToken" << errorReason;
            httpReplyAndClose(_socket, false, QStringLiteral("500 Internal Server Error"), tr("Login Error"), errorReason);
            Q_EMIT result(Error);
            return;
        }

        // this check is important when reauthenticating an existing account, where a davUser is already defined.
        // In this context, the check just validates that the user had not authenticated with a different username from the account
        if (!_davUser.isEmpty()) {
            checkUserInfo();
        } else { // davUser is empty so we are done:
            finalize(_accessToken, _refreshToken, _messageUrl);
        }
    });
}

void OAuth::checkUserInfo()
{
    Q_ASSERT(!_davUser.isEmpty());

    UserInfoAdapter infoAdapter(_networkAccessManager, _accessToken, _serverUrl);
    const UserInfoResult infoResult = infoAdapter.getResult();

    if (!infoResult.success()) {
        httpReplyAndClose(_socket, false, QStringLiteral("500 Internal Server Error"), tr("Login Error"), infoResult.error);
        Q_EMIT result(Error);
    } else {
        if (infoResult.userId.compare(_davUser, Qt::CaseInsensitive) != 0) {
            // Connected with the wrong user
            qCWarning(lcOauth) << "We expected the user" << _davUser << "but the server answered with user" << infoResult.userId;
            const QString message = tr("You logged-in with user <em>%1</em>, but must login with user <em>%2</em>.<br>"
                                       "Please return to the %3 client and restart the authentication.")
                                        .arg(infoResult.userId, _davUser, Theme::instance()->appNameGUI());
            httpReplyAndClose(_socket, false, QStringLiteral("403 Forbidden"), tr("Wrong user"), message);
            Q_EMIT result(Error);
        } else {
            finalize(_accessToken, _refreshToken, _messageUrl);
        }
    }
}

// todo: #24 - I think this should also be an adapter. I also have a vague recollection that we have a job that does this already
QNetworkReply *OAuth::postTokenRequest(QUrlQuery &&queryItems)
{
    const QUrl requestTokenUrl = _tokenEndpoint;
    QNetworkRequest req;
    req.setTransferTimeout(defaultTimeoutMs());
    switch (_endpointAuthMethod) {
    case TokenEndpointAuthMethods::client_secret_basic:
        req.setRawHeader("Authorization", "Basic " + QStringLiteral("%1:%2").arg(_clientId, _clientSecret).toUtf8().toBase64());
        break;
    case TokenEndpointAuthMethods::client_secret_post:
        queryItems.addQueryItem(QStringLiteral("client_id"), _clientId);
        queryItems.addQueryItem(QStringLiteral("client_secret"), _clientSecret);
        break;
    }
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded; charset=UTF-8"));
    req.setAttribute(DontAddCredentialsAttribute, true);

    queryItems.addQueryItem(QStringLiteral("scope"), QString::fromUtf8(QUrl::toPercentEncoding(Theme::instance()->openIdConnectScopes())));
    req.setUrl(requestTokenUrl);
    return _networkAccessManager->post(req, queryItems.toString(QUrl::FullyEncoded).toUtf8());
}

QByteArray OAuth::generateRandomString(size_t size) const
{
    // TODO: do we need a variable size?
    std::vector<quint32> buffer(size, 0);
    QRandomGenerator::global()->fillRange(buffer.data(), static_cast<qsizetype>(size));
    return QByteArray(reinterpret_cast<char *>(buffer.data()), static_cast<int>(size * sizeof(quint32))).toBase64(QByteArray::Base64UrlEncoding);
}

QUrl OAuth::authorisationLink() const
{
    Q_ASSERT(_server.isListening());
    Q_ASSERT(_wellKnownFinished);

    if (!_authEndpoint.isValid()) {
        return {}; // no auth endpoint, cannot continue
    }

    const QByteArray code_challenge =
        QCryptographicHash::hash(_pkceCodeVerifier, QCryptographicHash::Sha256).toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QUrlQuery query{{QStringLiteral("response_type"), QStringLiteral("code")}, {QStringLiteral("client_id"), _clientId},
        {QStringLiteral("redirect_uri"), QStringLiteral("%1:%2").arg(_redirectUrl, QString::number(_server.serverPort()))},
        {QStringLiteral("code_challenge"), QString::fromLatin1(code_challenge)}, {QStringLiteral("code_challenge_method"), QStringLiteral("S256")},
        {QStringLiteral("scope"), QString::fromUtf8(QUrl::toPercentEncoding(Theme::instance()->openIdConnectScopes()))},
        {QStringLiteral("prompt"), QString::fromUtf8(QUrl::toPercentEncoding(toString(_supportedPromptValues)))},
        {QStringLiteral("state"), QString::fromUtf8(_state)}};

    if (!_davUser.isEmpty()) {
        const QString davUser = QString::fromUtf8(QUrl::toPercentEncoding(_davUser)); // Issue #7762;
        // open id connect
        query.addQueryItem(QStringLiteral("login_hint"), davUser);
        // todo: #20 oc10 as fallback!
        query.addQueryItem(QStringLiteral("user"), davUser);
    }
    return Utility::concatUrlPath(_authEndpoint, {}, query);
}

void OAuth::fetchWellKnown()
{
    qCDebug(lcOauth) << "fetching" << wellKnownPathC;

    QNetworkRequest req;
    req.setAttribute(DontAddCredentialsAttribute, true);
    req.setUrl(Utility::concatUrlPath(_serverUrl, wellKnownPathC));
    req.setTransferTimeout(defaultTimeoutMs());

    auto reply = _networkAccessManager->get(req);

    QObject::connect(reply, &QNetworkReply::finished, this, [reply, this] {
        _wellKnownFinished = true;
        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(lcOauth) << "failed to fetch .well-known reply, error:" << reply->error();
            // Most likely the file does not exist, default to the normal endpoint
            Q_EMIT fetchWellKnownFinished();
            return;
        }
        QJsonParseError err = {};
        QJsonObject data = QJsonDocument::fromJson(reply->readAll(), &err).object();
        if (err.error == QJsonParseError::NoError) {
            _authEndpoint = QUrl::fromEncoded(data[QStringLiteral("authorization_endpoint")].toString().toUtf8());
            _tokenEndpoint = QUrl::fromEncoded(data[QStringLiteral("token_endpoint")].toString().toUtf8());
            _redirectUrl = QStringLiteral("http://127.0.0.1");

            const auto authMethods = data.value(QStringLiteral("token_endpoint_auth_methods_supported")).toArray();
            if (authMethods.contains(QStringLiteral("client_secret_basic"))) {
                _endpointAuthMethod = TokenEndpointAuthMethods::client_secret_basic;
            } else if (authMethods.contains(QStringLiteral("client_secret_post"))) {
                _endpointAuthMethod = TokenEndpointAuthMethods::client_secret_post;
            } else {
                OC_ASSERT_X(false, qPrintable(QStringLiteral("Unsupported token_endpoint_auth_methods_supported: %1").arg(QDebug::toString(authMethods))));
            }
            const auto promptValuesSupported = data.value(QStringLiteral("prompt_values_supported")).toArray();
            if (!promptValuesSupported.isEmpty()) {
                _supportedPromptValues = PromptValuesSupported::none;
                for (const auto &x : promptValuesSupported) {
                    const auto flag = Utility::stringToEnum<PromptValuesSupported>(x.toString());
                    // only use flags present in Theme::instance()->openIdConnectPrompt()
                    if (flag & defaultOauthPromptValue())
                        _supportedPromptValues |= flag;
                }
            }

            qCDebug(lcOauth) << "parsing .well-known reply successful, auth endpoint" << _authEndpoint << "and token endpoint" << _tokenEndpoint;
        } else if (err.error == QJsonParseError::IllegalValue) {
            qCDebug(lcOauth) << "failed to parse .well-known reply as JSON, server might not support OIDC";
        } else {
            qCDebug(lcOauth) << "failed to parse .well-known reply, error:" << err.error;
        }
        Q_EMIT fetchWellKnownFinished();
    });
}

/**
 * Checks whether a URL returned by the server is valid.
 * @param url URL to validate
 * @return true if validation is successful, false otherwise
 */
bool isUrlSchemeValid(const QUrl &url)
{
    qCDebug(lcOauth()) << "Checking URL scheme for validity:" << url;

    // todo: #21 - I am not sure this check has much of a point to begin with as
    // we have already validated any URL via user input OR we take the url from the webfinger service,
    // but if this is in fact "needed" it should check more than just the scheme imo

    // the following allow list contains URL schemes accepted as valid
    // OAuth 2.0 URLs must be HTTPS to be in compliance with the specification
    // for unit tests, we also permit the non existing oauthtest scheme
    const QStringList allowedSchemes({QStringLiteral("https"), QStringLiteral("oauthtest")});
    return allowedSchemes.contains(url.scheme());
}

void OAuth::openBrowser()
{
    auto authorisationURL = authorisationLink();
    if (authorisationURL.isEmpty()) {
        qCWarning(lcOauth) << "Authorization URL is unknown - well-known/openid-configuration endpoint did not return usable information";
        Q_EMIT result(ErrorIdPUnreachable, QString());
        return;
    }
    qCDebug(lcOauth) << "opening browser";

    if (!isUrlSchemeValid(authorisationURL)) {
        qCWarning(lcOauth) << "URL validation failed";
        Q_EMIT result(ErrorInsecureUrl, QString());
        return;
    }

    if (!QDesktopServices::openUrl(authorisationURL)) {
        qCWarning(lcOauth) << "QDesktopServices::openUrl Failed";
        // We cannot open the browser, then we claim we don't support OAuth.
        Q_EMIT result(NotSupported, QString());
    }
}

// todo: I was contemplating how we can make sure the passed account isn't null before we use it
// to seed the OAuth ctr, and really, I'm not sure this should be a subclass of oauth in the first place. Instead it could simply use an
// oauth instance to complete the tasks it can't do itself -> this could possibly be a "has a" not an "is a" impl
AccountBasedOAuth::AccountBasedOAuth(Account *account, QObject *parent)
    : OAuth(account->url(), account->davUser(), account->accessManager(), parent)
    , _account(account)
{
}

void AccountBasedOAuth::startAuthentication()
{
    OAuth::startAuthentication();
}

void AccountBasedOAuth::fetchWellKnown()
{
    if (!_account) {
        qCWarning(lcOauth) << "Unable to fetch well known, account has been deleted";
        return;
    }

    qCDebug(lcOauth) << "starting CheckServerJob before fetching" << wellKnownPathC;

    auto *checkServerJob = CheckServerJobFactory::createFromAccount(_account, true).startJob(_serverUrl, this);

    connect(checkServerJob, &CoreJob::finished, this, [checkServerJob, this]() {
        if (checkServerJob->success()) {
            qCDebug(lcOauth) << "CheckServerJob succeeded, fetching" << wellKnownPathC;
            OAuth::fetchWellKnown();
        } else {
            qCDebug(lcOauth) << "CheckServerJob failed, error:" << checkServerJob->errorMessage();
            if (_isRefreshingToken) {
                Q_EMIT refreshError(checkServerJob->reply()->error(), checkServerJob->errorMessage());
            } else {
                Q_EMIT result(Error);
            }
        }
    });
}

void AccountBasedOAuth::refreshAuthentication(const QString &refreshToken)
{
    if (!OC_ENSURE(!_isRefreshingToken)) {
        qCDebug(lcOauth) << "already refreshing token, aborting";
        return;
    }

    // I don't see where this ever gets set to false - seems to rely on a one shot run before creating a new instance where the value
    // is initialized to false. hm.
    _isRefreshingToken = true;


    // make this a real function -
    auto refresh = [this, refreshToken] {
        auto reply = postTokenRequest({{QStringLiteral("grant_type"), QStringLiteral("refresh_token")}, {QStringLiteral("refresh_token"), refreshToken}});
        connect(reply, &QNetworkReply::finished, this, [reply, refreshToken, this]() {
            const auto jsonData = reply->readAll();
            QJsonParseError jsonParseError;
            const auto data = QJsonDocument::fromJson(jsonData, &jsonParseError).object().toVariantMap();
            QString accessToken;
            QString newRefreshToken = refreshToken;
            // https://developer.okta.com/docs/reference/api/oidc/#response-properties-2
            const QString errorString = data.value(QStringLiteral("error")).toString();
            if (!errorString.isEmpty()) {
                if (errorString == QLatin1String("invalid_grant") || errorString == QLatin1String("invalid_request")) {
                    newRefreshToken.clear();
                } else {
                    qCWarning(lcOauth) << "Error while refreshing the token:" << errorString << data.value(QStringLiteral("error_description")).toString();
                }
            } else if (reply->error() != QNetworkReply::NoError) {
                qCWarning(lcOauth) << "Error while refreshing the token:" << reply->error() << ":" << reply->errorString()
                                   << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                Q_EMIT refreshError(reply->error(), reply->errorString());
                return;
            } else {
                if (jsonParseError.error != QJsonParseError::NoError || data.isEmpty()) {
                    // Invalid or empty JSON: Network error maybe?
                    qCWarning(lcOauth) << "Error while refreshing the token:" << jsonParseError.errorString();
                } else {
                    QString error;
                    accessToken = getRequiredField(data, QStringLiteral("access_token"), &error).toString();
                    if (!error.isEmpty()) {
                        qCWarning(lcOauth) << "The reply from the server did not contain all expected fields:" << error;
                    }

                    const auto refresh_token = data.find(QStringLiteral("refresh_token"));
                    if (refresh_token != data.constEnd()) {
                        newRefreshToken = refresh_token.value().toString();
                    }
                }
            }
            Q_EMIT refreshFinished(accessToken, newRefreshToken);
        });
    };

    connect(this, &OAuth::fetchWellKnownFinished, this, refresh);

    fetchWellKnown();
}

QString OCC::toString(OAuth::PromptValuesSupportedFlags s)
{
    QStringList out;
    for (auto k : {OAuth::PromptValuesSupported::consent, OAuth::PromptValuesSupported::select_account})
        if (s & k) {
            out += Utility::enumToString(k);
        }
    return out.join(QLatin1Char(' '));
}
