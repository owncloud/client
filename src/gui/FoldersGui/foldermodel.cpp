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

#include "foldermodel.h"


FolderModel::FolderModel(QObject *parent)
    : QAbstractItemModel{parent}
{
}

QVariant FolderModel::data(const QModelIndex &index, int role) const
{
    return {};
}


QModelIndex FolderModel::index(int row, int column, const QModelIndex &parent) const
{
    return {};
}

QModelIndex FolderModel::parent(const QModelIndex &index) const
{
    return {};
}

int FolderModel::rowCount(const QModelIndex &parent) const
{
    return 0;
}

int FolderModel::columnCount(const QModelIndex &parent) const
{
    return 3; // so far. maybe we get it from the item at that index instead
}
