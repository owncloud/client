/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "appprovider.h"

#include "common/utility.h"
#include "libsync/account.h"
#include "libsync/networkjobs/jsonjob.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QJsonArray>
#include <QMimeDatabase>

using namespace OCC;

Q_LOGGING_CATEGORY(lcAppProvider, "sync.appprovider", QtInfoMsg)

AppProvider::Provider::Provider(const QJsonObject &provider)
    : mimeType(provider.value(QStringLiteral("mime_type")).toString())
    , extension(provider.value(QStringLiteral("extension")).toString())
    , name(provider.value(QStringLiteral("name")).toString())
    , description(provider.value(QStringLiteral("description")).toString())
    , icon(provider.value(QStringLiteral("icon")).toString())
    , defaultApplication(provider.value(QStringLiteral("default_application")).toString())
    , allowCreation(provider.value(QStringLiteral("allow_creation")).toBool())
{
}

bool AppProvider::Provider::isValid() const
{
    return !mimeType.isEmpty();
}

AppProvider::AppProvider(const QJsonObject &apps)
{
    const auto mimTypes = apps.value(QStringLiteral("mime-types")).toArray();
    _providers.reserve(apps.size());
    for (const auto &type : mimTypes) {
        Provider p(type.toObject());
        _providers.insert(p.mimeType, std::move(p));
    }
}

const AppProvider::Provider &AppProvider::app(const QMimeType &mimeType) const
{
    if (auto it = Utility::optionalFind(_providers, mimeType.name())) {
        return it->value();
    }
    static const AppProvider::Provider nullProvider { {} };
    return nullProvider;
}

const AppProvider::Provider &AppProvider::app(const QString &localPath) const
{
    QMimeDatabase db;
    auto mimeType = db.mimeTypeForFile(localPath);
    return app(mimeType);
}

bool AppProvider::open(Account *account, const QString &localPath, const QByteArray &fileId) const
{
    if (!account)
        return false;

    const auto &a = app(localPath);
    if (a.isValid()) {
        SimpleNetworkJob::UrlQuery query { { QStringLiteral("file_id"), QString::fromUtf8(fileId) } };
        auto *job = new JsonJob(account, account->capabilities().appProviders().openWebUrl, {}, "POST", query);
        QObject::connect(job, &JsonJob::finishedSignal, [account, job, localPath] {
            if (job->httpStatusCode() == 200) {
                const auto url = QUrl(job->data().value(QStringLiteral("uri")).toString());
                const auto result = QDesktopServices::openUrl(url);
                qCDebug(lcAppProvider) << "start browser" << url << result;
            } else {
                Q_EMIT account->appProviderErrorOccured(
                    QCoreApplication::translate("AppProvider", "Failed to open %1 in web. Error: %2.").arg(localPath, job->reply()->errorString()));
            }
        });
        job->start();
        return true;
    }
    return false;
}
