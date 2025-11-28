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
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>

#include "theme.h"

namespace OCC {

AccountFoldersView::AccountFoldersView(QWidget *parent)
    : QWidget{parent}
{
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

    _addFolderButton = new QPushButton(tr("Add new folder sync..."), this);
    _addFolderButton->setObjectName("addAccountFolderButton");
    connect(_addFolderButton, &QPushButton::clicked, this, &AccountFoldersView::addFolderTriggered);
    buttonLineLayout->addStretch(1);
    buttonLineLayout->addWidget(_addFolderButton, 0, Qt::AlignRight);
    mainLayout->addLayout(buttonLineLayout);

    _treeView = new QTreeView(this);
    _treeView->setHeaderHidden(true);
    _treeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // I'm not sure always off is good but that's what we currently have
    _treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _treeView->setLineWidth(2);
    mainLayout->addWidget(_treeView);

    _syncedFolderCountLabel = new QLabel("placeholder for sync count", this);
    _syncedFolderCountLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLayout->addWidget(_syncedFolderCountLabel, 0, Qt::AlignLeft);

    setLayout(mainLayout);
}

void AccountFoldersView::setItemModel(QStandardItemModel *model)
{
    _treeView->setModel(model);
}

void AccountFoldersView::setSyncedFolderCount(int synced, int total) { }
}
