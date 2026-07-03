/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
