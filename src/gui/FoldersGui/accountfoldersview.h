/*
 * Copyright (C) Lisa Reese <lisa.reese@kiteworks.com>
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

class QStandardItemModel;
class QTreeView;
class QLabel;
class QPushButton;

namespace OCC {

class AccountFoldersView : public QWidget
{
    Q_OBJECT
public:
    explicit AccountFoldersView(QWidget *parent = nullptr);

    void setItemModel(QStandardItemModel *model);
    void setFolderActions(QList<QAction *> actions);
    void setSyncedFolderCount(int synced, int total);
    void enableAddFolder(bool enableAdd);

signals:
    void addFolderTriggered();

private:
    void buildView();

    QTreeView *_treeView = nullptr;
    QLabel *_syncedFolderCountLabel = nullptr;
    QPushButton *_addFolderButton = nullptr;
};
}
