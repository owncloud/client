/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
