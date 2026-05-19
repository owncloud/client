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
#include "aboutview.h"
#include "ui_aboutview.h"

#include "gui/guiutility.h"
#include "libsync/theme.h"

// #include <QMessageBox>

namespace OCC {

AboutView::AboutView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AboutView)
{
    ui->setupUi(this);
    ui->aboutText->setText(Theme::instance()->about());
    ui->icon->setPixmap(Theme::instance()->aboutIcon().pixmap(256));
    ui->versionInfo->setText(Theme::instance()->aboutVersions(Theme::VersionFormat::RichText));

    connect(ui->versionInfo, &QTextBrowser::anchorClicked, this, &AboutView::openBrowserFromUrl);
    connect(ui->aboutText, &QLabel::linkActivated, this, &AboutView::openBrowser);
}

AboutView::~AboutView()
{
    delete ui;
}

void AboutView::openBrowser(const QString &s)
{
    Utility::openBrowser(QUrl(s), this);
}

void AboutView::openBrowserFromUrl(const QUrl &s)
{
    return openBrowser(s.toString());
}

} // OCC namespace
