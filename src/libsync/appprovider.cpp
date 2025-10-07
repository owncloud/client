/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
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
