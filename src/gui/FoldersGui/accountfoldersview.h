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

class QStandardItemModel;
class QItemSelectionModel;
class QTreeView;
class QLabel;
class QPushButton;
class QMenu;

namespace OCC {

class AccountFoldersView : public QWidget
{
    Q_OBJECT
public:
    explicit AccountFoldersView(QWidget *parent = nullptr);

    void setItemModels(QStandardItemModel *model, QItemSelectionModel *selectionModel);
    void setFolderActions(QList<QAction *> actions);
    void setSyncedFolderCount(int synced, int total);
    void enableAddFolder(bool enableAdd);
    void setMenuActions(QList<QAction *> actions);

signals:
    void addFolderTriggered();
    void requestActionsUpdate();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    void buildView();
    void popItemMenu(const QPoint &pos);
    void refreshMenu();
    bool performBizarreSetupOnTreeView();

    QTreeView *_treeView = nullptr;
    QLabel *_syncedFolderCountLabel = nullptr;
    QPushButton *_addFolderButton = nullptr;
    QMenu *_itemMenu = nullptr;
    bool _firstShowAfterCreation = true;
};
}
