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
#pragma once

#include <QIdentityProxyModel>
#include <QObject>

#include <functional>

namespace OCC::Models {

class VirtualElementsProxyModel : public QIdentityProxyModel
{
    Q_OBJECT
public:
    struct ExtraData
    {
        std::function<QVariant(int, int)> data = [](int column, int role) -> QVariant {
            return {};
        };
        std::function<Qt::ItemFlags(int)> flags = [](int columng) -> Qt::ItemFlags {
            return {};
        };
    };

    using QIdentityProxyModel::QIdentityProxyModel;

    void addVirtualElement(ExtraData &&data);

public:
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;

    QVector<ExtraData> _extraData;

    // QAbstractItemModel interface
public:
    QVariant data(const QModelIndex &index, int role) const override;

    // QAbstractItemModel interface
public:
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // QAbstractItemModel interface
public:
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;


    // QAbstractItemModel interface
public:
    bool hasChildren(const QModelIndex &parent) const override;
};

}
