/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SYNCRUNFILELOG_H
#define SYNCRUNFILELOG_H

#include <QFile>
#include <QTextStream>
#include <QScopedPointer>
#include <QElapsedTimer>

#include "syncfileitem.h"

namespace OCC {
class SyncFileItem;

/**
 * @brief The SyncRunFileLog class
 * @ingroup gui
 */
class SyncRunFileLog
{
public:
    SyncRunFileLog();
    void start(const QString &folderPath);
    void logItem(const SyncFileItem &item);
    void logLap(const QString &name);
    void finish();

protected:
private:
    QScopedPointer<QFile> _file;
    QElapsedTimer _totalDuration;
    QElapsedTimer _lapDuration;
    QScopedPointer<QTextStream> _out;
};
}

#endif // SYNCRUNFILELOG_H
