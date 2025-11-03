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

#include <QDialog>
#include <QLocale>

#include "owncloudgui.h"
#include "models/protocolitemmodel.h"

#include "models/models.h"

class QSortFilterProxyModel;
class QTableView;

namespace OCC {

namespace Ui {
    class LocalActivityWidget;
}

/**
 * @brief The LocalActivityWidget class
 * @ingroup gui
 */
class LocalActivityWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LocalActivityWidget(QWidget *parent = nullptr);
    ~LocalActivityWidget() override;

    // todo: these absolutely do not belong here. move them to an independent impl which can be shared between the protocolWidget and the issuesWidget
    static void showContextMenu(QWidget *parent, QTableView *table, Models::SignalledQSortFilterProxyModel *sortModel, ProtocolItemModel *itemModel,
        const QModelIndexList &items, const QPoint &pos);
    static QMenu *showFilterMenu(QWidget *parent, Models::SignalledQSortFilterProxyModel *model, int role, const QString &columnName);

public Q_SLOTS:
    void slotItemCompleted(Folder *folder, const SyncFileItemPtr &item);
    void filterDidChange();

private Q_SLOTS:
    void slotItemContextMenu(const QPoint &pos);

    void onFolderListChanged();

    void onFolderRemoved(Folder *f);

private:
    ProtocolItemModel *_model;
    Models::SignalledQSortFilterProxyModel *_sortModel;
    Ui::LocalActivityWidget *_ui;
};
}
