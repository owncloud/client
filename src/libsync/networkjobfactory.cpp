/*
 * Copyright (C) by Olivier Goffart <ogoffart@owncloud.com>
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

#include "networkjobfactory.h"

/** \file networkjobfactory.cpp
 *
 * \brief Class that creates network jobs on demand.
 * Creates an normal or encrypted network job based on context.
 *
 * Overview
 * --------
 *
 * TODO
 *
 */

namespace OCC {

MkColJob* NetworkJobFactory::createMkColJob(AccountPtr account, const QString &path, QObject *parent)
{
    if (! _helper.isFileEncrypted(path))
        return new MkColJob(account, path, parent);
    else
        // for now helper.isFileEncrypted returns always false so this is never executed.
        // TODO orion : update once the encrypted class is created.
        return 0;
}

LsColJob* NetworkJobFactory::createLsColJob(AccountPtr account, const QString &path, QObject *parent)
{
    if (! _helper.isFileEncrypted(path))
        return new LsColJob(account, path, parent);
    else
        // for now helper.isFileEncrypted returns always false so this is never executed.
        // TODO orion : update once the encrypted class is created.
        return 0;
}

RequestEtagJob* NetworkJobFactory::createRequestEtagJob(AccountPtr account, const QString &path, QObject *parent)
{
    if (! _helper.isFileEncrypted(path))
        return new RequestEtagJob(account, path, parent);
    else
        // for now helper.isFileEncrypted returns always false so this is never executed.
        // TODO orion : update once the encrypted class is created.
        return 0;
}

PropfindJob* NetworkJobFactory::createPropfindJob(AccountPtr account, const QString &path, QObject *parent)
{
    if (! _helper.isFileEncrypted(path))
        return new PropfindJob(account, path, parent);
    else
        // for now helper.isFileEncrypted returns always false so this is never executed.
        // TODO orion : update once the encrypted class is created.
        return 0;
}

ProppatchJob* NetworkJobFactory::createProppatchJob(AccountPtr account, const QString &path, QObject *parent)
{
    if (! _helper.isFileEncrypted(path))
        return new ProppatchJob(account, path, parent);
    else
        // for now helper.isFileEncrypted returns always false so this is never executed.
        // TODO orion : update once the encrypted class is created.
        return 0;
}

EntityExistsJob* NetworkJobFactory::createEntityExistsJob(AccountPtr account, const QString &path, QObject *parent)
{
    if (! _helper.isFileEncrypted(path))
        return new EntityExistsJob(account, path, parent);
    else
        // for now helper.isFileEncrypted returns always false so this is never executed.
        // TODO orion : update once the encrypted class is created.
        return 0;
}

JsonApiJob* NetworkJobFactory::createJsonApiJob(AccountPtr account, const QString &path, QObject *parent)
{
    if (! _helper.isFileEncrypted(path))
        return new JsonApiJob(account, path, parent);
    else
        // for now helper.isFileEncrypted returns always false so this is never executed.
        // TODO orion : update once the encrypted class is created.
        return 0;
}

GETFileJob* NetworkJobFactory::createGETFileJob(AccountPtr account, const QString& path, QFile *device,
                             const QMap<QByteArray, QByteArray> &headers, const QByteArray &expectedEtagForResume,
                             quint64 resumeStart, QObject* parent)
{
    if (! _helper.isFileEncrypted(path))
        return new GETFileJob(account, path, device, headers, expectedEtagForResume, resumeStart, parent);
    else
        // for now helper.isFileEncrypted returns always false so this is never executed.
        // TODO orion : update once the encrypted class is created.
        return 0;
}

GETFileJob* NetworkJobFactory::createGETFileJob(AccountPtr account, const QUrl& url, QFile *device,
                             const QMap<QByteArray, QByteArray> &headers, const QByteArray &expectedEtagForResume,
                             quint64 resumeStart, QObject* parent)
{
    if (! _helper.isFileEncrypted(url))
        return new GETFileJob(account, url, device, headers, expectedEtagForResume, resumeStart, parent);
    else
        // for now helper.isFileEncrypted returns always false so this is never executed.
        // TODO orion : update once the encrypted class is created.
        return 0;
}

PUTFileJob* NetworkJobFactory::createPUTFileJob(AccountPtr account, const QString& path, QIODevice *device,
                             const QMap<QByteArray, QByteArray> &headers, int chunk, QObject* parent)
{
    if (! _helper.isFileEncrypted(path))
        return new PUTFileJob(account, path, device, headers, chunk, parent);
    else
        // for now helper.isFileEncrypted returns always false so this is never executed.
        // TODO orion : update once the encrypted class is created.
        return 0;
}

/**
 * @brief NetworkJobFactory::createPollJob
 * Creates either a normal or an encrypted ThumbnailJob network job, depending on path.
 * @param account : passed as is to the relevant constructor
 * @param path : Used to decide whether to create an encrypted or normal version of the network job,
 * and passed as is to the relevant constructor
 * @param item : passed as is to the relevant constructor
 * @param journal : passed as is to the relevant constructor
 * @param localPath : passed as is to the relevant constructor
 * @param parent : passed as is to the relevant constructor
 * @return A pointer to the created network job. Will be either normal or encrypted.
 */
PollJob* NetworkJobFactory::createPollJob(AccountPtr account, const QString &path, const SyncFileItemPtr &item,
                       SyncJournalDb *journal, const QString &localPath, QObject *parent)
{
    if (! _helper.isFileEncrypted(path))
        return new PollJob(account, path, item, journal, localPath, parent);
    else
        // for now helper.isFileEncrypted returns always false so this is never executed.
        // TODO orion : update once the encrypted class is created.
        return 0;
}

/**
 * \brief Used by the factory to determine whether a job should be created in
 * its normal version or an encrypted version.
 *
 * Overview
 * --------
 *
 * TODO
 *
 */
bool OWNCLOUDSYNC_EXPORT NetworkJobFactoryHelper::isFileEncrypted(SyncFileItem *item)
{
    // TODO orion : for now always return false, we only want to create normal networkjob.
    Q_UNUSED(item)
    return false;
}

bool OWNCLOUDSYNC_EXPORT NetworkJobFactoryHelper::isFileEncrypted(const QString &path)
{
    // TODO orion : for now always return false, we only want to create normal networkjob.
    Q_UNUSED(path)
    return false;
}

bool OWNCLOUDSYNC_EXPORT NetworkJobFactoryHelper::isFileEncrypted(const QUrl &url)
{
    // TODO orion : for now always return false, we only want to create normal networkjob.
    Q_UNUSED(url)
    return false;
}

}
