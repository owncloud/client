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

#include "accountfoldersview.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>

#include "buttondelegate.h"
#include "folderitemdelegate.h"
#include "theme.h"

namespace OCC {


AccountFoldersView::AccountFoldersView(QWidget *parent)
    : QWidget{parent}
{
    // important to know: the parent in this ctr is always null because the widget is instantiated by the ui impl. it leaves the parent
    // null presumably because it adds it to a stacked widget after ctr which takes ownership. The issue with this is that you *can't*
    // do anything relative to the parent in this construction setup because it is not known at time of construction.
    // this makes setting the palette pretty icky
    _itemMenu = new QMenu(this);
    _itemMenu->setAccessibleName(tr("Sync options menu"));

    buildView();
}

void AccountFoldersView::buildView()
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    QLabel *titleLabel = new QLabel(tr("Folder sync"), this);
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLayout->addWidget(titleLabel, 0, Qt::AlignLeft);

    QHBoxLayout *buttonLineLayout = new QHBoxLayout();
    QLabel *description = new QLabel(tr("Manage your synced folders"), this);
    description->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    buttonLineLayout->addWidget(description, 0, Qt::AlignLeft);

    _addFolderButton = new QPushButton(tr("Add new folder sync…"), this);
    _addFolderButton->setObjectName("addAccountFolderButton");
    connect(_addFolderButton, &QPushButton::clicked, this, &AccountFoldersView::addFolderTriggered);
    buttonLineLayout->addStretch(1);
    buttonLineLayout->addWidget(_addFolderButton, 0, Qt::AlignRight);
    mainLayout->addLayout(buttonLineLayout);

    _treeView = new QTreeView(this);
    _treeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // I'm not sure always off is good but that's what we currently have
    _treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _treeView->setIndentation(0);
    _treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    _treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    _treeView->setAllColumnsShowFocus(true);

    _treeView->header()->setSectionsMovable(false);
    _treeView->setHeaderHidden(true);

    FolderItemDelegate *delegate = new FolderItemDelegate(_treeView);
    _treeView->setItemDelegateForColumn(0, delegate);
    // note this is not the normal ellipses character, it's vertically centered instead of positioned at font baseline. This is better
    // for this button than normal ellipses
    ButtonDelegate *buttonDel = new ButtonDelegate("⋯", _treeView);
    buttonDel->setMenu(_itemMenu);
    _treeView->setItemDelegateForColumn(1, buttonDel);
    // this works but only when the button item is current. this means I have to set the button column to current each time row selection
    // changes but it works well so far. Still needs some tweaks to ensure the edit mode stays active when you click around in the same row.
    // sometimes the button disappears. Note that using AllEditTriggers does not improve things but with all triggers you can trigger the menu
    // on the button by hitting the space bar - this seems really weird but I'll take it? Needs testing on windows.
    _treeView->setEditTriggers(QAbstractItemView::AllEditTriggers);

    _treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(_treeView, &QWidget::customContextMenuRequested, this, &AccountFoldersView::popItemMenu);

    mainLayout->addWidget(_treeView);

    _syncedFolderCountLabel = new QLabel("placeholder for sync count", this);
    _syncedFolderCountLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLayout->addWidget(_syncedFolderCountLabel, 0, Qt::AlignLeft);

    setLayout(mainLayout);
}

void AccountFoldersView::setItemModels(QStandardItemModel *model, QItemSelectionModel *selectionModel)
{
    // order here is important, if you set the selection model before the main model the selection model for the view
    // reverts to the "default" selectionModel.
    // also the tree view doesn't manage the lifetime of the selection  model so we could/possibly should delete the default here
    _treeView->setModel(model);
    _treeView->setSelectionModel(selectionModel);

    // these settings can only work if/when the model has been added to the view because the header "data" comes from the model.
    // without the header data the logical indexes do not exist so...we can't do it in the ctr unless we inject the models there.
    // but we can't do that either as the view is currently created by the .ui impl, which can't take those params.
    QHeaderView *header = _treeView->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
}

void AccountFoldersView::setSyncedFolderCount(int synced, int total)
{
    _syncedFolderCountLabel->setText(tr("Syncing %1 out of %2 spaces").arg(QString::number(synced), QString::number(total)));
}

void AccountFoldersView::enableAddFolder(bool enableAdd)
{
    _addFolderButton->setEnabled(enableAdd);
}

void AccountFoldersView::setFolderActions(QList<QAction *> actions)
{
    if (_itemMenu) {
        _itemMenu->clear();
        _itemMenu->addActions(actions);
        connect(_itemMenu, &QMenu::aboutToShow, this, &AccountFoldersView::refreshMenu);
    }
}

void AccountFoldersView::refreshMenu()
{
    emit requestActionsUpdate();
}

void AccountFoldersView::popItemMenu(const QPoint &pos)
{
    _itemMenu->exec(_treeView->viewport()->mapToGlobal(pos));
}
}
