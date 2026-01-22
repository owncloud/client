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

#include <QStandardItem>

namespace OCC {

class FolderItem;

/**
 * @brief The FolderErrorItem class will present any errors that arise on sync
 *
 * the error items will be children of the related FolderItem
 *
 */
class FolderErrorItem : public QStandardItem
{
    FolderErrorItem(FolderItem *parent);

private:
    FolderItem *_parent;
};
}
