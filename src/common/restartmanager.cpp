/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "restartmanager.h"

#include "utility.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QProcess>
#include <QTimer>

Q_LOGGING_CATEGORY(lcRestart, "sync.restartmanager", QtInfoMsg)

using namespace OCC;

RestartManager *RestartManager::_instance = nullptr;

RestartManager::RestartManager(std::function<int(int, char **)> &&main)
    : _main(main)
{
    Q_ASSERT(!_instance);
    if (!_instance)
        _instance = this;
}

RestartManager::~RestartManager()
{
    if (!_applicationToRestart.isEmpty()) {
        QProcess process;
        process.setProgram(_applicationToRestart);
        process.setArguments(_args);
        qint64 pid;
        qCDebug(lcRestart) << "Detaching" << _applicationToRestart << _args;
        if (process.startDetached(&pid)) {
            qCDebug(lcRestart) << "Successfully restarted. New process PID" << pid;
        } else {
            qCCritical(lcRestart) << "Failed to restart" << process.error() << process.errorString();
        }
    }
}

int RestartManager::exec(int argc, char **argv) const
{
    return _main(argc, argv);
}

void RestartManager::requestRestart()
{
    Q_ASSERT(_instance);
    if (!_instance)
        return;

    qCInfo(lcRestart) << "Restarting application with PID" << QCoreApplication::applicationPid();

    QString pathToLaunch = QCoreApplication::applicationFilePath();
#ifdef Q_OS_LINUX
    if (Utility::runningInAppImage()) {
        pathToLaunch = Utility::appImageLocation();
    }
#endif
    _instance->_applicationToRestart = QFileInfo(pathToLaunch).absoluteFilePath();
    // remove arg0
    _instance->_args = QCoreApplication::arguments().sliced(1);
    QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
}
