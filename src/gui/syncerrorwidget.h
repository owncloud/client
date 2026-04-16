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

#include <QWidget>

#include "progressdispatcher.h"

namespace OCC {

class SyncFileItemStatusSetSortFilterProxyModel;
class ProtocolItemModel;
class Folder;

namespace Models {
    class SignalledQSortFilterProxyModel;
}

namespace Ui {
    class SyncErrorWidget;
}

/**
 * @brief The SyncErrorWidget class
 * @ingroup gui
 */
class SyncErrorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SyncErrorWidget(QWidget *parent = nullptr);
    ~SyncErrorWidget() override;

public Q_SLOTS:
    void slotProgressInfo(Folder *folder, const ProgressInfo &progress);
    void slotItemCompleted(Folder *folder, const SyncFileItemPtr &item);
    void filterChanged();

Q_SIGNALS:
    void issueCountUpdated(int);

private Q_SLOTS:
    QMenu *showFilterMenu(QWidget *parent);
    void slotItemContextMenu(const QPoint &pos);
    void onFolderListChanged(const QUuid &accountId, const QList<Folder *> folders);
    void onFolderRemoved(const QUuid &accountId, Folder *f);

private:
    static void addResetFiltersAction(QMenu *menu, const QList<std::function<void()>> &resetFunctions);
    std::function<void()> addStatusFilter(QMenu *menu);

    ProtocolItemModel *_model;
    Models::SignalledQSortFilterProxyModel *_sortModel;
    SyncFileItemStatusSetSortFilterProxyModel *_statusSortModel;

    Ui::SyncErrorWidget *_ui;
};
}
