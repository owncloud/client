/*
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
#include "removeallfileswidget.h"
#include "ui_removeallfileswidget.h"

#include "resources/resources.h"

using namespace OCC;

RemoveAllFilesWidget::RemoveAllFilesWidget(SyncFileItem::Direction direction, const QString &path, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RemoveAllFilesWidget)
{
    ui->setupUi(this);

    ui->trashIcon->setPixmap(Resources::getCoreIcon(QStringLiteral("delete")).pixmap({128, 128}));
    if (direction == SyncFileItem::Down) {
        ui->label->setText(tr("All files in the sync folder '%1' folder were deleted on the server.\n"
                              "These deletes will be synchronized to your local sync folder, making such files "
                              "unavailable unless you have a right to restore. \n"
                              "If you decide to keep the files, they will be re-synced with the server if you have rights to do so.\n"
                              "If you decide to delete the files, they will be unavailable to you, unless you are the owner.")
                               .arg(path));
    } else {
        ui->label->setText(tr("All the files in your local sync folder '%1' were deleted. These deletes will be "
                              "synchronized with your server, making such files unavailable unless restored.\n"
                              "Are you sure you want to sync those actions with the server?\n"
                              "If this was an accident and you decide to keep your files, they will be re-synced from the server.")
                               .arg(path));
    }
}

RemoveAllFilesWidget::~RemoveAllFilesWidget()
{
    delete ui;
}
