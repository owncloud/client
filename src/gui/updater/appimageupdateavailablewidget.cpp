/*
 * Copyright (C) 2022 by Fabian Müller <fmueller@owncloud.com>
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

#include "appimageupdateavailablewidget.h"
#include "ui_appimageupdateavailablewidget.h"

#include "theme.h"

#include "resources/resources.h"

#include <QStyle>

namespace OCC {

AppImageUpdateAvailableWidget::AppImageUpdateAvailableWidget(const QVersionNumber &currentVersion, const QVersionNumber &newVersion, QWidget *parent)
    : QWidget(parent)
    , _ui(new Ui::AppImageUpdateAvailableWidgetUi)
{
    _ui->setupUi(this);

    const auto *theme = Theme::instance();

    // the strings in the .ui file are not marked for translation, they're just placeholders
    _ui->installedVersionLabel->setText(tr("Installed version: %1").arg(currentVersion.toString()));
    _ui->availableVersionLabel->setText(tr("Available update: %1").arg(newVersion.toString()));
    _ui->infoLabel->setText(tr("An update is available for this AppImage of %1. Do you want to install this update?\n\nThe update will be performed in the "
                               "background, and overwrite the current AppImage file. You need to restart the app to complete the update.")
                                .arg(theme->appNameGUI()));

    _ui->appIconLabel->setPixmap(theme->aboutIcon().pixmap(QSize(128, 128)));

    // we use custom icons to ensure a unified look on all platforms
    _ui->buttonBox->button(QDialogButtonBox::Ok)->setIcon(Resources::getCoreIcon(QStringLiteral("check")));
    _ui->buttonBox->button(QDialogButtonBox::Cancel)->setIcon(Resources::getCoreIcon(QStringLiteral("ban")));
    _ui->skipButton->setIcon(Resources::getCoreIcon(QStringLiteral("step-forward")));

    // the minimum size of the info label (and a few other labels) depends on their contents
    // we can't persuade the dialog to resize automatically to the recommended size in Qt Designer, so we do it manually
    resize(sizeHint());
    // also, we want to prevent users from reducing the widget size too much, i.e., widgets would be hidden partially
    setMinimumSize(sizeHint());

    connect(_ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &AppImageUpdateAvailableWidget::accepted);
    connect(_ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &AppImageUpdateAvailableWidget::rejected);
    connect(_ui->skipButton, &QPushButton::clicked, this, &AppImageUpdateAvailableWidget::skipUpdateButtonClicked);
}

AppImageUpdateAvailableWidget::~AppImageUpdateAvailableWidget()
{
    delete _ui;
}

}
