/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
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

#include "gui/owncloudguilib.h"

#include "accountstate.h"
#include "activitydata.h"

#include <QtCore>

class QJsonDocument;

namespace OCC {

Q_DECLARE_LOGGING_CATEGORY(lcActivity)

/**
 * @brief The ActivityListModel
 * @ingroup gui
 *
 * Simple list model to provide the list view with data.
 */

class OWNCLOUDGUI_EXPORT ActivityListModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum class ActivityRole {
        Text,
        Account,
        PointInTime,
        Path,

        ColumnCount
    };
    Q_ENUM(ActivityRole)


    explicit ActivityListModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;

    bool canFetchMore(const QModelIndex &) const override;
    void fetchMore(const QModelIndex &) override;

    ActivityList activityList() { return _finalList; }


public Q_SLOTS:
    void slotRefreshActivity(AccountState *ast);
    void slotRemoveAccount(AccountState *ast);

Q_SIGNALS:
    void activityJobStatusCode(AccountState *ast, int statusCode);

private:
    void setActivityList(const ActivityList &&resultList);
    void startFetchJob(AccountState *ast);
    void combineActivityLists();

    QMap<AccountState *, ActivityList> _activityLists;
    ActivityList _finalList;
    QMap<AccountState *, AbstractNetworkJob *> _currentlyFetching;

    friend class TestActivityModel;
};
}
