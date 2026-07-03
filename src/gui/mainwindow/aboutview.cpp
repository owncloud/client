/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
