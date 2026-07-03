/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "syncfileitem.h"
#include <QLoggingCategory>
#include <QNetworkReply>

namespace OCC {

inline QString getEtagFromReply(QNetworkReply *reply)
{
    QByteArray rawEtag = reply->rawHeader("OC-ETag");
    if (rawEtag.isEmpty()) {
        rawEtag = reply->rawHeader("ETag");
    }
    return Utility::normalizeEtag(QString::fromUtf8(rawEtag));
}

/**
 * Given an error from the network, map to a SyncFileItem::Status error
 */
inline SyncFileItem::Status classifyError(QNetworkReply::NetworkError nerror,
    int httpCode, bool *anotherSyncNeeded = nullptr, const QByteArray &errorBody = QByteArray())
{
    Q_ASSERT(nerror != QNetworkReply::NoError); // we should only be called when there is an error

    if (nerror == QNetworkReply::RemoteHostClosedError) {
        // Sometimes server bugs lead to a connection close on certain files,
        // that shouldn't bring the rest of the syncing to a halt.
        return SyncFileItem::NormalError;
    }

    if (nerror > QNetworkReply::NoError && nerror <= QNetworkReply::UnknownProxyError) {
        // network error or proxy error -> fatal
        return SyncFileItem::FatalError;
    }

    switch (httpCode) {
    case 423:
        // "Locked"
        // Should be temporary.
        if (anotherSyncNeeded != nullptr) {
            *anotherSyncNeeded = true;
        }
        return SyncFileItem::Message;
    case 425:
        // "Too Early"
        // The file is currently post processed after an upload
        // once the post processing finished we get a new etag and retry
        return SyncFileItem::Message;
    case 502:
        // "Bad Gateway"
        // Should be temporary.
        if (anotherSyncNeeded != nullptr) {
            *anotherSyncNeeded = true;
        }
        Q_FALLTHROUGH();
    case 412:
        // "Precondition Failed"
        // Happens when the e-tag has changed
        return SyncFileItem::SoftError;
    case 503: {
        // When the server is in maintenance mode, we want to exit the sync immediatly
        // so that we do not flood the server with many requests
        // BUG: This relies on a translated string and is thus unreliable.
        //      In the future it should return a NormalError and trigger a status.php
        //      check that detects maintenance mode reliably and will terminate the sync run.
        auto probablyMaintenance =
                errorBody.contains(R"(>Sabre\DAV\Exception\ServiceUnavailable<)")
                && !errorBody.contains("Storage is temporarily not available");
        return probablyMaintenance ? SyncFileItem::FatalError : SyncFileItem::NormalError;
    }
    }
    return SyncFileItem::NormalError;
}
}
