/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
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

#include <QAbstractItemModel>

namespace OCC::GraphApi {
class SpacesManager;
class Space;
};

namespace OCC::Spaces {
class SpacesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum class Roles {
        AccessibleDescriptionRole = Qt::AccessibleDescriptionRole,
        IsSynced = Qt::UserRole + 1,
        Name,
        Subtitle,
        WebUrl,
        WebDavUrl,
        Priority,
        Enabled,
        Space,
    };
    Q_ENUM(Roles)
    explicit SpacesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setSpacesManager(GraphApi::SpacesManager *spacesManager);

private:
    GraphApi::SpacesManager *_spacesManager = nullptr;
    QVector<GraphApi::Space *> _spacesList;
};
}
