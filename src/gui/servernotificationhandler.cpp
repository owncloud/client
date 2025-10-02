/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
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

#include "servernotificationhandler.h"
#include "accountstate.h"
#include "capabilities.h"
#include "creds/abstractcredentials.h"
#include "networkjobs/jsonjob.h"

#include <QJsonObject>

namespace OCC {

Q_LOGGING_CATEGORY(lcServerNotification, "gui.servernotification", QtInfoMsg)

const QString notificationsPath = QStringLiteral("ocs/v2.php/apps/notifications/api/v1/notifications");

ServerNotificationHandler::ServerNotificationHandler(QObject *parent)
    : QObject(parent)
{
}

void ServerNotificationHandler::slotFetchNotifications(AccountState *accountState)
{
    // check connectivity and credentials
    if (!(accountState && accountState->isConnected() && accountState->account() && accountState->account()->credentials()
            && accountState->account()->credentials()->ready())) {
        deleteLater();
        return;
    }
    // check if the account has notifications enabled. If the capabilities are
    // not yet valid, its assumed that notifications are available.
    if (accountState->account()->hasCapabilities()) {
        if (!accountState->account()->capabilities().notificationsAvailable()) {
            qCInfo(lcServerNotification) << "Account" << accountState->account()->displayNameWithHost() << "does not have notifications enabled.";
            deleteLater();
            return;
        }
    } else {
        deleteLater();
        return;
    }

    // if the previous notification job has finished, start next.
    auto *job = new JsonApiJob(accountState->account(), notificationsPath, {}, {}, this);
    QObject::connect(job, &JsonApiJob::finishedSignal, this, [job, accountState, this] {
        slotNotificationsReceived(job, accountState);
        deleteLater();
    });

    job->start();
}

void ServerNotificationHandler::slotNotificationsReceived(JsonApiJob *job, AccountState *accountState)
{
    if (!accountState || !accountState->account())
        return;

    if (job->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200) {
        qCWarning(lcServerNotification) << "Notifications failed with status code " << job->ocsStatus();
        return;
    }

    const auto &notifies = job->data().value(QLatin1String("ocs")).toObject().value(QLatin1String("data")).toArray();

    ActivityList list;
    list.reserve(notifies.size());
    for (const auto &element : notifies) {
        const auto json = element.toObject();
        const QString id = json.value(QStringLiteral("notification_id")).toVariant().toString();

        const auto actions = json.value(QStringLiteral("actions")).toArray();
        QVector<ActivityLink> linkList;
        linkList.reserve(actions.size() + 1);
        for (const auto &action : actions) {
            const auto actionJson = action.toObject();
            ActivityLink al;
            al._label = QUrl::fromPercentEncoding(actionJson.value(QStringLiteral("label")).toString().toUtf8());
            al._link = actionJson.value(QStringLiteral("link")).toString();
            al._verb = actionJson.value(QStringLiteral("type")).toString().toUtf8();
            al._isPrimary = actionJson.value(QStringLiteral("primary")).toBool();
            linkList.append(al);
        }

        // Add another action to dismiss notification on server
        // https://github.com/owncloud/notifications/blob/master/docs/ocs-endpoint-v1.md#deleting-a-notification-for-a-user
        ActivityLink al;
        al._label = tr("Dismiss");
        al._link = Utility::concatUrlPath(Utility::concatUrlPath(accountState->account()->url(), notificationsPath), id).toString();
        al._verb  = "DELETE";
        al._isPrimary = false;
        linkList.append(al);

        list.append(Activity{Activity::NotificationType, id, accountState->account()->displayNameWithHost(), accountState->account()->uuid(),
            json.value(QStringLiteral("subject")).toString(), json.value(QStringLiteral("message")).toString(), QString(),
            QUrl(json.value(QStringLiteral("link")).toString()), QDateTime::fromString(json.value(QStringLiteral("datetime")).toString(), Qt::ISODate),
            std::move(linkList)});
    }
    Q_EMIT newNotificationList(list);
}
}
