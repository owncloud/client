/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.org>
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
#include "config.h"
#include "logger.h"

#include "folderman.h"
#include "account.h"
#include "accountstate.h"

#include "folderdefinition.h"

#include <QSettings>
#include <QDir>

namespace OCC {

Q_LOGGING_CATEGORY(lcFolderDef, "gui.folderdefinition", QtInfoMsg)


void FolderDefinition::save(QSettings &settings, const FolderDefinition &folder)
{
    settings.beginGroup(FolderMan::escapeAlias(folder.alias));
    settings.setValue(QLatin1String("localPath"), folder.localPath);
    settings.setValue(QLatin1String("journalPath"), folder.journalPath);
    settings.setValue(QLatin1String("targetPath"), folder.targetPath);
    settings.setValue(QLatin1String("paused"), folder.paused);
    settings.setValue(QLatin1String("ignoreHiddenFiles"), folder.ignoreHiddenFiles);
    settings.endGroup();
}

bool FolderDefinition::load(QSettings &settings, const QString &alias,
    FolderDefinition *folder)
{
    settings.beginGroup(alias);
    folder->alias = FolderMan::unescapeAlias(alias);
    folder->localPath = settings.value(QLatin1String("localPath")).toString();
    folder->journalPath = settings.value(QLatin1String("journalPath")).toString();
    folder->targetPath = settings.value(QLatin1String("targetPath")).toString();
    folder->paused = settings.value(QLatin1String("paused")).toBool();
    folder->ignoreHiddenFiles = settings.value(QLatin1String("ignoreHiddenFiles"), QVariant(true)).toBool();
    folder->cryptoContainerPath = settings.value(QLatin1String("cryptoContainerPath")).toString();
    settings.endGroup();

    // Old settings can contain paths with native separators. In the rest of the
    // code we assum /, so clean it up now.
    folder->localPath = prepareLocalPath(folder->localPath);

    // Target paths also have a convention
    folder->targetPath = prepareTargetPath(folder->targetPath);

    return true;
}

QString FolderDefinition::prepareLocalPath(const QString &path)
{
    QString p = QDir::fromNativeSeparators(path);
    if (!p.endsWith(QLatin1Char('/'))) {
        p.append(QLatin1Char('/'));
    }
    return p;
}

QString FolderDefinition::prepareTargetPath(const QString &path)
{
    QString p = path;
    if (p.endsWith(QLatin1Char('/'))) {
        p.chop(1);
    }
    // Doing this second ensures the empty string or "/" come
    // out as "/".
    if (!p.startsWith(QLatin1Char('/'))) {
        p.prepend(QLatin1Char('/'));
    }
    return p;
}

QString FolderDefinition::absoluteJournalPath() const
{
    return QDir(localPath).filePath(journalPath);
}

QString FolderDefinition::defaultJournalPath(AccountPtr account)
{
    return SyncJournalDb::makeDbName(localPath, account->url(), targetPath, account->credentials()->user());
}

} // namespace OCC
