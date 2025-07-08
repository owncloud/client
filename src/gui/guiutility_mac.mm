/*
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
 * Copyright (C) by Erik Verbruggen <erik@verbruggen.consulting>
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

#include "application.h"
#include "guiutility.h"

#include "libsync/theme.h"

#include <QProcess>

#import <Foundation/NSBundle.h>

namespace OCC {

void Utility::startShellIntegration()
{
    QString bundlePath = QUrl::fromNSURL([NSBundle mainBundle].bundleURL).path();

    auto _system = [](const QString &cmd, const QStringList &args) {
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(cmd, args);
        if (!process.waitForFinished()) {
            qCWarning(lcGuiUtility) << "Failed to load shell extension:" << cmd
                                    << args.join(QLatin1Char(' ')) << process.errorString();
        } else {
            qCInfo(lcGuiUtility) << (process.exitCode() != 0 ? "Failed to load" : "Loaded")
                                 << "shell extension:" << cmd << args.join(QLatin1Char(' '))
                                 << process.readAll();
        }
    };

    // Add it again. This was needed for Mojave to trigger a load.
    _system(QStringLiteral("pluginkit"), { QStringLiteral("-a"), QStringLiteral("%1Contents/PlugIns/FinderSyncExt.appex/").arg(bundlePath) });

    // Tell Finder to use the Extension (checking it from System Preferences -> Extensions)
    _system(QStringLiteral("pluginkit"),
        {QStringLiteral("-e"), QStringLiteral("use"), QStringLiteral("-i"), Theme::instance()->orgDomainName() + QStringLiteral(".FinderSyncExt")});
}

QString Utility::socketApiSocketPath()
{
    // This must match the code signing Team setting of the extension
    // Example for developer builds (with ad-hoc signing identity): "" "com.seagate.curator.stxfiles.macos" ".socketApi"
    // Example for official signed packages: "9B5WD74GWJ." "com.seagate.curator.stxfiles.macos" ".socketApi"
    return QStringLiteral("%1%2.socketApi").arg(QStringLiteral(SOCKETAPI_TEAM_IDENTIFIER_PREFIX), Theme::instance()->orgDomainName());
}

} // namespace OCC
