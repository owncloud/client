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

#ifndef FOLDERDEFINITION_H
#define FOLDERDEFINITION_H

#include <QObject>
#include <QStringList>

class AccountPtr;
class QSettings;

namespace OCC {

/**
 * @brief The FolderDefinition class
 * @ingroup gui
 */
class FolderDefinition
{
public:
    FolderDefinition()
        : paused(false)
        , ignoreHiddenFiles(true)
    {
    }

    /// The name of the folder in the ui and internally
    QString alias;
    /// path on local machine
    QString localPath;
    /// path to the journal, usually relative to localPath
    QString journalPath;
    /// path on remote
    QString targetPath;
    /// whether the folder is paused
    bool paused;
    /// whether the folder syncs hidden files
    bool ignoreHiddenFiles;
    /// path to the crypto container if the folder is an encrypted one
    QString cryptoContainerPath;

    /// Saves the folder definition, creating a new settings group.
    static void save(QSettings &settings, const FolderDefinition &folder);

    /// Reads a folder definition from a settings group with the name 'alias'.
    static bool load(QSettings &settings, const QString &alias,
        FolderDefinition *folder);

    /// Ensure / as separator and trailing /.
    static QString prepareLocalPath(const QString &path);

    /// Ensure starting / and no ending /.
    static QString prepareTargetPath(const QString &path);

    /// journalPath relative to localPath.
    QString absoluteJournalPath() const;

    /// Returns the relative journal path that's appropriate for this folder and account.
    QString defaultJournalPath(AccountPtr account);
};

}

#endif
