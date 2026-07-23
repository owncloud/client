/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QDate>

#include "syncrunfilelog.h"
#include "common/utility.h"
#include "filesystem.h"
#include <qfileinfo.h>

namespace {
auto dateTimeStr(const QDateTime &dt = QDateTime::currentDateTimeUtc())
{
    return dt.toString(Qt::ISODate);
}

}
namespace OCC {

SyncRunFileLog::SyncRunFileLog()
{
}


void SyncRunFileLog::start(const QString &folderPath)
{
    const qint64 logfileMaxSize = 10 * 1024 * 1024; // 10MiB

    // Note; this name is ignored in csync_exclude.c
    const QString filename = folderPath + QStringLiteral(".owncloudsync.log");

    // When the file is too big, just rename it to an old name.
    QFileInfo info(filename);
    bool exists = info.exists();
    if (exists && info.size() > logfileMaxSize) {
        exists = false;
        QString newFilename = filename + QStringLiteral(".1");
        QFile::remove(newFilename);
        QFile::rename(filename, newFilename);
    }
    _file.reset(new QFile(filename));
    _file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);


    // we use a text stream to ensure the encoding is ok
    // when outputting info, we use QDebug to ensure we can use the debug operators
    _out.reset(new QTextStream(_file.data()));
    _out->setEncoding(QStringConverter::Utf8);


    if (!exists) {
        // We are creating a new file, add the note.
        *_out << "Log for:" << folderPath << Qt::endl
              << "# timestamp | duration | file | instruction | dir | modtime | etag | "
                 "size | fileId | status | errorString | http result code | "
                 "other size | other modtime | X-Request-ID"
              << Qt::endl;

        FileSystem::setFileHidden(filename, true);
    }


    _totalDuration.start();
    _lapDuration.start();
    *_out << "#=#=#=# Syncrun started " << dateTimeStr() << Qt::endl;
}

void SyncRunFileLog::logItem(const SyncFileItem &item)
{
    // don't log the directory items that are in the list
    if (item._direction == SyncFileItem::None || item.instruction() == CSYNC_INSTRUCTION_IGNORE) {
        return;
    }
    const QChar L = QLatin1Char('|');
    QString tmp;
    {
        QDebug(&tmp).noquote() << dateTimeStr(Utility::parseRFC1123Date(QString::fromUtf8(item._responseTimeStamp))) << L
                               << ((item.instruction() != CSYNC_INSTRUCTION_RENAME) ? item.destination()
                                                                                    : item._file + QStringLiteral(" -> ") + item._renameTarget)
                               << L << item.instruction() << L << item._direction << L << L << item._modtime << L << item._etag << L << item._size << L
                               << item._fileId << L << item._status << L << item._errorString << L << item._httpErrorCode << L << item._previousSize << L
                               << item._previousModtime << L << item._requestId << L << Qt::endl;
    }
    *_out << tmp;
}

void SyncRunFileLog::logLap(const QString &name)
{
    QString tmp;
    {
        QDebug(&tmp).noquote() << "#=#=#=#=#" << name << dateTimeStr() << "(last step:" << _lapDuration.restart() << "msec"
                               << ", total:" << _totalDuration.elapsed() << "msec)" << Qt::endl;
    }
    *_out << tmp;
}

void SyncRunFileLog::finish()
{
    QString tmp;
    {
        QDebug(&tmp).noquote() << "#=#=#=# Syncrun finished" << dateTimeStr() << "(last step:" << _lapDuration.elapsed() << "msec"
                               << ", total:" << _totalDuration.elapsed() << "msec)" << Qt::endl;
    }
    *_out << tmp;
    _out->flush();
    _out->reset();
    _file->close();
}
}
