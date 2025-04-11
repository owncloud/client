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
#include "spacemigration.h"

#include "common/version.h"
#include "gui/accountmanager.h"
#include "gui/folderman.h"
#include "libsync/account.h"
#include "libsync/graphapi/jobs/drives.h"

#include <QJsonArray>
#include <QSettings>

Q_LOGGING_CATEGORY(lcMigration, "gui.migration.spaces", QtInfoMsg)

using namespace OCC;

SpaceMigration::SpaceMigration(const AccountStatePtr &account, const QString &path, QObject *parent)
    : QObject(parent)
    , _accountState(account)
    , _path(path)
{
}

void SpaceMigration::start()
{
    // Refactoring todo: this should never be called by anyone but the FolderMan. In fact this spaces migration thing should
    // probably be managed by the folder man since it is responsible for other migration tasks, but at the moment it's managed by the AccountState
    FolderMan::instance()->setSyncEnabled(false);

    QJsonArray folders;
    for (auto *f : FolderMan::instance()->folders()) {
        // same account and uses the legacy dav url
        // already migrated folders are ignored
        if (f->accountState()->account() == _accountState->account() && f->webDavUrl() == _accountState->account()->davUrl()) {
            folders.append(f->remotePath());
            _migrationFolders.append(f);
        }
    }
    const QJsonObject obj{{QStringLiteral("version"), Version::version().toString()}, {QStringLiteral("user"), _accountState->account()->davUser()},
        {QStringLiteral("remotefolder"), folders}};
    auto job = new JsonJob(_accountState->account(), _accountState->account()->url(), _path, QByteArrayLiteral("POST"), obj);
    job->setParent(this);
    connect(job, &JsonJob::finishedSignal, this, [job, this] {
        const int status = job->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 200 && job->parseError().error == QJsonParseError::NoError) {
            migrate(job->data().value(QStringLiteral("folders")).toObject());
        } else {
            if (job->parseError().error != QJsonParseError::NoError) {
                qCInfo(lcMigration) << job->parseError().errorString();
            }
            Q_EMIT finished();
        }
    });
    job->start();
}

void SpaceMigration::migrate(const QJsonObject &folders)
{
    auto drivesJob = new GraphApi::Drives(_accountState->account(), this);
    // refactoring todo: clean up
    connect(drivesJob, &GraphApi::Drives::finishedSignal, this, [drivesJob, folders, this] {
        const auto drives = drivesJob->drives();
        // note this is going to sync when it goes out of scope. We do *not* want to
        // sync after each folder def save as it's file access is a relatively expensive operation
        // and it's overkill to do it every time, especially if there are loads of folders.
        // concerning some case where it crashes mid-migration, I don't think there is any value
        // at all in having some subset of the migration saved if it didn't finish successfully, is there?
        auto settings = _accountState->settings();
        settings->beginGroup(QStringLiteral("Folders"));
        for (auto &folder : _migrationFolders) {
            if (folder) {
                const auto obj = folders.value(folder->remotePath()).toObject();
                const QString error = obj.value(QStringLiteral("error")).toString();
                if (error.isEmpty()) {
                    const QString newPath = obj.value(QStringLiteral("path")).toString();
                    const QString space_id = obj.value(QStringLiteral("space_id")).toString();

                    const auto it = std::find_if(drives.cbegin(), drives.cend(), [space_id](auto it) { return it.getId() == space_id; });

                    if (it != drives.cend()) {
                        qCDebug(lcMigration) << "Migrating:" << folder->path() << "davUrl:" << folder->_definition.webDavUrl() << "->"
                                             << it->getRoot().getWebDavUrl() << "remotPath:" << folder->_definition.targetPath() << "->" << newPath;
                        folder->_definition.setWebDavUrl(QUrl(it->getRoot().getWebDavUrl()));
                        folder->_definition.setTargetPath(newPath);
                        auto id = QString::fromUtf8(folder->_definition.id());
                        settings->beginGroup(id);
                        FolderDefinition::save(*(settings.get()), folder->definition());
                        settings->endGroup();
                    }
                } else {
                    qCInfo(lcMigration) << "No migration of" << folder->remotePath() << "reason:" << error;
                }
            }
        }
        _accountState->_supportsSpaces = true;
        AccountManager::instance()->save();
        FolderMan::instance()->setSyncEnabled(true);
        Q_EMIT finished();
    });
    drivesJob->start();
}
