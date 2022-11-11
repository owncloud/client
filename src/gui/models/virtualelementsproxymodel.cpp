/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "virtualelementsproxymodel.h"
#include <QDebug>

using namespace OCC::Models;

void VirtualElementsProxyModel::addVirtualElement(ExtraData &&data)
{
    _extraData.append(std::move(data));
}

QModelIndex VirtualElementsProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    return QIdentityProxyModel::index(row, column, parent);
}

QModelIndex VirtualElementsProxyModel::parent(const QModelIndex &child) const
{
    const auto realSize = QIdentityProxyModel::rowCount();
    if (child.row() >= realSize) {
        return {};
    }
    return QIdentityProxyModel::parent(child);
}

int VirtualElementsProxyModel::rowCount(const QModelIndex &parent) const
{
    return QIdentityProxyModel::rowCount(parent) + _extraData.size();
}

QVariant VirtualElementsProxyModel::data(const QModelIndex &index, int role) const
{
    const auto realSize = QIdentityProxyModel::rowCount();
    if (index.row() >= realSize) {
        return _extraData.at(index.row() - realSize).data(index.column(), role);
    }
    return QIdentityProxyModel::data(index, role);
}

Qt::ItemFlags VirtualElementsProxyModel::flags(const QModelIndex &index) const
{
    const auto realSize = QIdentityProxyModel::rowCount();
    if (index.row() >= realSize) {
        return _extraData.at(index.row() - realSize).flags(index.column());
    }
    return QIdentityProxyModel::flags(index);
}

QModelIndex VirtualElementsProxyModel::sibling(int row, int column, const QModelIndex &idx) const
{
    const auto realSize = QIdentityProxyModel::rowCount();
    if (row >= realSize) {
        return index(row, column, {});
    }
    return QIdentityProxyModel::sibling(row, column, idx);
}

bool VirtualElementsProxyModel::hasChildren(const QModelIndex &parent) const
{
    const auto realSize = QIdentityProxyModel::rowCount();
    if (parent.row() >= realSize) {
        return false;
    }
    return QIdentityProxyModel::hasChildren(parent);
}
