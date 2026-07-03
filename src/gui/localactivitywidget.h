/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QDialog>
#include <QLocale>

// #include "owncloudgui.h"
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

    void onFolderListChanged(const QUuid &accountId, const QList<Folder *> folders);

    void onFolderRemoved(const QUuid &accountId, Folder *f);

private:
    ProtocolItemModel *_model;
    Models::SignalledQSortFilterProxyModel *_sortModel;
    Ui::LocalActivityWidget *_ui;
};
}
