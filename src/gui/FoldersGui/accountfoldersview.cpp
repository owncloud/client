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

    // I think this needs to move to a slot connected to QHeaderView::sectionCountChanged  - in theory the logical indexes
    // will have been created by that point?!
    QHeaderView *header = _treeView->header();
    header->setSectionsMovable(false);
    header->setStretchLastSection(false); // this is useless - so far the last section still stretches
    //_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch); // ->this does not work/crashes.I think the "logical index" does not yet exist but
    // WHEN does it get created?! I try updating the header after the model is set and it crashes there too. I do not get it.
    header->setSectionResizeMode(QHeaderView::Stretch); // this at least splits the available space between first and second column.
    _treeView->setHeaderHidden(true);


    FolderItemDelegate *delegate = new FolderItemDelegate(_treeView);
    _treeView->setItemDelegateForColumn(0, delegate);
    ButtonDelegate *buttonDel = new ButtonDelegate(_treeView);
    buttonDel->setMenu(_itemMenu);
    _treeView->setItemDelegateForColumn(1, buttonDel);

    // this works but only when the button item is current. this means I have to set the button column to current each time row selection
    // changes but it works well so far.
    _treeView->setEditTriggers(QAbstractItemView::CurrentChanged);
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
    // I'll check that out
    _treeView->setModel(model);
    _treeView->setSelectionModel(selectionModel);    
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
    }
}

void AccountFoldersView::popItemMenu(const QPoint &pos)
{
    emit requestActionsUpdate();

    _itemMenu->exec(_treeView->viewport()->mapToGlobal(pos));
}
}
