/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <QDateTime>
#include <QFile>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QSet>
#include <QTextStream>

#include "owncloudlib.h"

namespace OCC {

/**
 * @brief The Logger class
 * @ingroup libsync
 */
class OWNCLOUDSYNC_EXPORT Logger : public QObject
{
    Q_OBJECT
public:
    static QString loggerPattern();

    bool isLoggingToFile() const;

    void attacheToConsole();

    void doLog(QtMsgType type, const QMessageLogContext &ctx, const QString &message);

    static Logger *instance();

    void setLogFile(const QString &name);
    void setLogDir(const QString &dir);
    void setLogFlush(bool flush);

    /**
     * Set the maximum number of logs files to keep.
     * Setting values below 5 will have no effect.
     */
    void setMaxLogFiles(int i);

    bool logDebug() const { return _logDebug; }
    void setLogDebug(bool debug);

    /** Returns where the automatic logdir would be */
    QString temporaryFolderLogDirPath() const;

    /** Sets up default dir log setup.
     *
     * logdir: a temporary folder
     * logdebug: true
     *
     * Used in conjunction with ConfigFile::automaticLogDir,
     * see LogBrowser::setupLoggingFromConfig.
     */
    void setupTemporaryFolderLogDir();

    /** For switching off via logwindow */
    void disableTemporaryFolderLogDir();

    void addLogRule(const QSet<QString> &rules) {
        setLogRules(_logRules + rules);
    }
    void removeLogRule(const QSet<QString> &rules) {
        setLogRules(_logRules - rules);
    }
    void setLogRules(const QSet<QString> &rules);

private:
    Logger(QObject *parent = nullptr);
    ~Logger() override;

    void rotateLog();

    void open(const QString &name);
    void close();
    void dumpCrashLog();

    QFile _logFile;
    bool _doFileFlush = false;
    bool _logDebug = false;
    QScopedPointer<QTextStream> _logstream;
    mutable QRecursiveMutex _mutex;
    QString _logDirectory;
    bool _temporaryFolderLogDir = false;
    QSet<QString> _logRules;
    QVector<QString> _crashLog;
    int _crashLogIndex = 0;
    bool _consoleIsAttached = false;

    int _maxLogFiles;
};

} // namespace OCC

#endif // LOGGER_H
