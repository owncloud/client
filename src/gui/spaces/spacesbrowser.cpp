﻿/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
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
#include "spacesbrowser.h"
#include "ui_spacesbrowser.h"

#include "spacesdelegate.h"
#include "spacesmodel.h"


#include "graphapi/drives.h"

#include "gui/models/expandingheaderview.h"
#include "gui/models/models.h"

#include <QCursor>
#include <QMenu>
#include <QSortFilterProxyModel>

using namespace OCC::Spaces;

SpacesBrowser::SpacesBrowser(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SpacesBrowser)
{
    ui->setupUi(this);
    _model = new SpacesModel(this);

    auto *sortModel = new OCC::Models::WeightedQSortFilterProxyModel(this);
    sortModel->setSourceModel(_model);
    sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    sortModel->setWeightedColumn(static_cast<int>(SpacesModel::Columns::Priority));

    ui->tableView->setModel(sortModel);

    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SpacesBrowser::selectionChanged);

    ui->tableView->setItemDelegate(new SpacesDelegate);
    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    auto *header = new OCC::ExpandingHeaderView(QStringLiteral("SpacesBrowserHeader2"), ui->tableView);
    ui->tableView->setHorizontalHeader(header);
    header->setResizeToContent(true);
    header->setSortIndicator(static_cast<int>(SpacesModel::Columns::Name), Qt::DescendingOrder);
    header->setExpandingColumn(static_cast<int>(SpacesModel::Columns::Name));
    header->hideSection(static_cast<int>(SpacesModel::Columns::WebDavUrl));
    // part of the name (see the delegate)
    header->hideSection(static_cast<int>(SpacesModel::Columns::Subtitle));
    header->hideSection(static_cast<int>(SpacesModel::Columns::Priority));
    header->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header, &QHeaderView::customContextMenuRequested, header, [header, this] {
        auto menu = new QMenu(this);
        menu->setAttribute(Qt::WA_DeleteOnClose);
        header->addResetActionToMenu(menu);
        menu->popup(QCursor::pos());
    });
}

SpacesBrowser::~SpacesBrowser()
{
    delete ui;
}

void SpacesBrowser::setAccount(OCC::AccountPtr acc)
{
    _acc = acc;
    if (acc) {
        QTimer::singleShot(0, this, [this] {
            auto drive = new OCC::GraphApi::Drives(_acc);
            connect(drive, &OCC::GraphApi::Drives::finishedSignal, this, [drive, this] {
                auto drives = drive->drives();

                // hide disabled spaces
                drives.erase(std::remove_if(drives.begin(), drives.end(), [](const OpenAPI::OAIDrive &drive) {
                    // this is how disabled spaces are represented in the graph API
                    return drive.getRoot().getDeleted().getState() == QStringLiteral("trashed");
                }));

                _model->setDriveData(_acc, drives);
                show();
            });
            drive->start();
        });
    }
}

QModelIndex SpacesBrowser::currentSpace()
{
    const auto spaces = ui->tableView->selectionModel()->selectedRows();
    return spaces.isEmpty() ? QModelIndex {} : spaces.first();
}
