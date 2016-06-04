/*
 * Copyright (C) by orion1024 <orion1024+src@use.startmail.com>
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

#ifndef NETWORKJOBFACTORY_H
#define NETWORKJOBFACTORY_H

#include <QObject>
#include <QFile>

#include "networkjobs.h"
#include "syncfileitem.h"
#include "owncloudlib.h"

namespace OCC {

class GETFileJob;
class PUTFileJob;
class PollJob;
class SyncJournalDb;
class DeleteJob;
class MoveJob;

/**
 * @brief The NetworkJobFactoryHelper class
 * Provides helper methods to factory classes.
 */
class OWNCLOUDSYNC_EXPORT NetworkJobFactoryHelper : public QObject {
    Q_OBJECT
public:
    NetworkJobFactoryHelper(QObject *parent=0)
        : QObject(parent) {}

    bool isFileEncrypted(SyncFileItem *item);
    bool isFileEncrypted(const QString &path);
    bool isFileEncrypted(const QUrl &url);

};

/**
 * @brief Will handle reading from and writing to metadata information to permanent storage
 * @ingroup libowncrypt
 */
class OWNCLOUDSYNC_EXPORT NetworkJobFactory : public QObject {
    Q_OBJECT
public:

    NetworkJobFactory(QObject *parent=0)
        : QObject(parent) {}

    MkColJob* createMkColJob(AccountPtr account, const QString &path, QObject *parent = 0);
    LsColJob* createLsColJob(AccountPtr account, const QString &path, QObject *parent = 0);
    RequestEtagJob* createRequestEtagJob(AccountPtr account, const QString &path, QObject *parent = 0);
    PropfindJob* createPropfindJob(AccountPtr account, const QString &path, QObject *parent = 0);
    ProppatchJob* createProppatchJob(AccountPtr account, const QString &path, QObject *parent = 0);
    EntityExistsJob* createEntityExistsJob(AccountPtr account, const QString &path, QObject *parent = 0);
    JsonApiJob* createJsonApiJob(AccountPtr account, const QString &path, QObject *parent = 0);

    GETFileJob* createGETFileJob(AccountPtr account, const QString& path, QFile *device,
                                 const QMap<QByteArray, QByteArray> &headers, const QByteArray &expectedEtagForResume,
                                 quint64 resumeStart, QObject* parent = 0);
    GETFileJob* createGETFileJob(AccountPtr account, const QUrl& url, QFile *device,
                                 const QMap<QByteArray, QByteArray> &headers, const QByteArray &expectedEtagForResume,
                                 quint64 resumeStart, QObject* parent = 0);

    PUTFileJob* createPUTFileJob(AccountPtr account, const QString& path, QIODevice *device,
                                 const QMap<QByteArray, QByteArray> &headers, int chunk, QObject* parent = 0);

    PollJob* createPollJob(AccountPtr account, const QString &path, const SyncFileItemPtr &item,
                           SyncJournalDb *journal, const QString &localPath, QObject *parent);

    DeleteJob* createDeleteJob(AccountPtr account, const QString& path, QObject* parent = 0);

    MoveJob* createMoveJob(AccountPtr account, const QString& path, const QString &destination, QObject* parent = 0);

private:
    NetworkJobFactoryHelper _helper;

};

}

#endif
