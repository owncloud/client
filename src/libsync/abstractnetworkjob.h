/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
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

#include "accountfwd.h"
#include "jobqueue.h"

#include "common/asserts.h"

#include "owncloudlib.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QUrlQuery>

#include <chrono>
#include <optional>

class QUrl;


OWNCLOUDSYNC_EXPORT QDebug operator<<(QDebug debug, const OCC::AbstractNetworkJob *job);

namespace OCC {

using HeaderMap = QMap<QByteArray, QByteArray>;

/**
 * @brief The AbstractNetworkJob class
 * @ingroup libsync
 */
class OWNCLOUDSYNC_EXPORT AbstractNetworkJob : public QObject
{
    Q_OBJECT
public:
    explicit AbstractNetworkJob(AccountPtr account, const QUrl &baseUrl, const QString &path, QObject *parent = nullptr);
    ~AbstractNetworkJob() override;

    virtual void start();

    AccountPtr account() const { return _account; }
    QString path() const { return _path; }

    /*
     * A base Url, for most of the jobs this will be the WebDAV entry point.
     */
    QUrl baseUrl() const;

    /*
     * The absolute url: baseUrl() + path() + query()
     */
    QUrl url() const;

    QNetworkReply *reply() const;

    void setForceIgnoreCredentialFailure(bool ignore);
    bool ignoreCredentialFailure() const;

    /**
     * @brief Returns the timestamp from the reply
     *
     * @return Empty if the job was aborted, or an error occurred and no response was received.
     *         Otherwise the timestamp from the reply header. Note that the timestamp might be
     *         absent if the reply didn't contain one.
     */
    QByteArray responseTimestamp() const;
    /**
     * See responseTimestamp(), now converted to a QDateTime. If there was no response timestamp,
     * return an invalid `QDateTime`.
     */
    QDateTime responseQTimeStamp() const;

    int httpStatusCode() const;

    /* Content of the X-Request-ID header. (Only set after the request is sent) */
    QByteArray requestId();

    auto timeoutSec() const { return _timeout; }
    bool timedOut() const { return _timedout; }
    bool aborted() const { return _aborted; }

    void setPriority(QNetworkRequest::Priority priority);
    QNetworkRequest::Priority priority() const;

    /** Returns an error message, if any. */
    QString errorString() const;

    /** Like errorString, but also checking the reply body for information.
     *
     * Specifically, sometimes xml bodies have extra error information.
     * This function reads the body of the reply and parses out the
     * error information, if possible.
     *
     * \a body is optinally filled with the reply body.
     *
     * Warning: Needs to call reply()->readAll().
     */
    QString errorStringParsingBody(QByteArray *body = nullptr);

    /** Make a new request */
    void retry();

    /** Abort the job due to an error */
    void abort();

    /** static variable the HTTP timeout. If set to 0, the default will be used
     */
    static std::chrono::seconds httpTimeout;

    /**
     * The default 5 minutes timeout if none is specified by the config.
     * Qt's default would be 30s.
     */
    static constexpr std::chrono::seconds DefaultHttpTimeout { 5 * 60 };

    /** whether or noth this job should be restarted after authentication */
    bool  isAuthenticationJob() const;
    void  setAuthenticationJob(bool b);

    /** How many times was that job retried */
    int retryCount() const { return _retryCount; }


    virtual bool needsRetry() const;

    void setTimeout(const std::chrono::seconds sec);

    /**
     * Configure whether to store replies in a cache configured in the corresponding network access manager (if one is set there).
     * By default, replies are not stored in the cache. Qt normally would store all eligible resources (as configured by server policies) in the cache.
     * See also \ref setCacheLoadControl
     * @param storeInCache whether to store replies in the cache
     */
    void setStoreInCache(bool storeInCache);

    /**
     * Configure whether to load data from the cache.
     * We inherit Qt's default behavior (which in Qt 5.15 and 6.4 is to prefer a network response but provide a cached value if the server permits it in its
     * cache policies when the server cannot be reached).
     * See also \ref setStoreInCache
     * @param cacheLoadControl a policy suitable for the current job
     */
    void setCacheLoadControl(QNetworkRequest::CacheLoadControl cacheLoadControl);

Q_SIGNALS:
    /** Emitted on network error.
     *
     * \a reply is never null
     */
    void networkError(QNetworkReply *reply);

    /**
     * The job is done
     */
    void aboutToFinishSignal(QPrivateSignal);
    void finishedSignal(QPrivateSignal);

protected:
    /** Initiate a network request, returning a QNetworkReply.
     *
     * Calls setReply() and setupConnections() on it.
     *
     * Takes ownership of the requestBody (to allow redirects).
     */
    void sendRequest(const QByteArray &verb,
        const QNetworkRequest &req = QNetworkRequest(),
        QIODevice *requestBody = nullptr);

    /** Can be used by derived classes to set up the network reply.
     *
     * Particularly useful when the request is redirected and reply()
     * changes. For things like setting up additional signal connections
     * on the new reply.
     */
    virtual void newReplyHook(QNetworkReply *) {}

    /** Called at the end of QNetworkReply::finished processing.
     */
    virtual void finished() = 0;

    QByteArray _responseTimestamp;

    QString replyStatusString();

    /*
     * The url query appended to the url.
     * The query will not be set as part of the body.
     * The query must be fully encoded.
     */
    void setQuery(const QUrlQuery &query);
    QUrlQuery query() const;

    AccountPtr _account;

private:
    /** Makes this job drive a pre-made QNetworkReply
     *
     * This reply cannot have a QIODevice request body because we can't get
     * at it and thus not resend it in case of redirects.
     */
    void adoptRequest(QPointer<QNetworkReply> reply);
    void slotFinished();

    const QUrl _baseUrl;
    const QString _path;

    QUrlQuery _query;

    std::chrono::seconds _timeout = httpTimeout;
    bool _timedout = false; // set to true when the timeout slot is received
    bool _aborted = false;
    bool _finished = false;
    bool _forceIgnoreCredentialFailure = false;

    QNetworkRequest _request;
    QByteArray _verb;
    QPointer<QNetworkReply> _reply; // (QPointer because the NetworkManager may be destroyed before the jobs at exit)

    // Set by the xyzRequest() functions and needed to be able to redirect
    // requests, should it be required.
    //
    // Reparented to the currently running QNetworkReply.
    QPointer<QIODevice> _requestBody;

    bool _isAuthenticationJob = false;
    int _retryCount = 0;

    // by default, we don't intend to store responses in the cache (if one is set in the account's access manager)
    bool _storeInCache = false;
    // we use Qt's default cache load behavior unless the user explicitly requests a different behavior
    std::optional<QNetworkRequest::CacheLoadControl> _cacheLoadControl = std::nullopt;

    QNetworkRequest::Priority _priority = QNetworkRequest::NormalPriority;

    friend QDebug(::operator<<)(QDebug debug, const AbstractNetworkJob *job);
};


/** Gets the SabreDAV-style error message from an error response.
 *
 * This assumes the response is XML with a 'error' tag that has a
 * 'message' tag that contains the data to extract.
 *
 * Returns a null string if no message was found.
 */
QString OWNCLOUDSYNC_EXPORT extractErrorMessage(const QByteArray &errorResponse);

/** Builds a error message based on the error and the reply body. */
QString OWNCLOUDSYNC_EXPORT errorMessage(const QString &baseError, const QByteArray &body);

/** Nicer errorString() for QNetworkReply
 *
 * By default QNetworkReply::errorString() often produces messages like
 *   "Error downloading <url> - server replied: <reason>"
 * but the "downloading" part invariably confuses people since the
 * error might very well have been produced by a PUT request.
 *
 * This function produces clearer error messages for HTTP errors.
 */
QString OWNCLOUDSYNC_EXPORT networkReplyErrorString(const QNetworkReply &reply);

} // namespace OCC
