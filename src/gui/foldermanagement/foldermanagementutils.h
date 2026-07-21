/*
 * Copyright (C) Lisa Reese <lisa.reese@kiteworks.com>
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

#pragma once

#include <QCoreApplication>
#include <QString>

namespace OCC {
/**
 * @brief The FolderManagementUtils class is a "controlled" collection of general functions related to managing folders and their paths.
 *
 * indeed, this could be a namespace but I have seen too many abuses of "a namespace can be defined ANYWHERE" to be comfortable with it
 *
 * Declaring a final, non instantiable class prevents any creative "namespace" decls/defs
 *
 */
class FolderManagementUtils final
{
    Q_DECLARE_TR_FUNCTIONS(FolderManagementUtils)

public:
    FolderManagementUtils() = delete;
    ~FolderManagementUtils() = delete;

    /**
     * @brief prepareFolder creates the folder from given path if it does not already exist, and configures the folder with mac and windows
     * specific operations
     * @param folder path
     * @return true if the folder path exists and was successfully configured
     */
    static bool prepareFolder(const QString &path);

    /**
     * @brief numberOfSyncJournals counts sync journals by matching folder content with .sync_*.db or ._sync_*.db in the target folder
     * @param path is the folder to check for number of sync journals
     * @return the number of sync journals found in the given folder.
     */
    static qsizetype numberOfSyncJournals(const QString &path);

    /**
     *  verifies that the length of the given path does not exceed system limits
     *  if the path fails the check, the return value will contain an error string
     *  if it passes, the return value will be empty
     **/
    static QString checkPathLength(const QString &path);

    /**
     *  performs various checks on the folder path to ensure it exists and can be used as local sync destination.
     *  if the checks fail, the return value will contain the error
     *  if it passes, the return value will be empty.
     **/
    static QString validateFolderPath(const QString &path);
};
}
