/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "logbrowser.h"

#include "stdio.h"
#include <iostream>

#include <QDesktopServices>
#include <QDir>
#include <QLayout>
#include <QMessageBox>
#include <QTextStream>
#include <optional>

#include "configfile.h"
#include "guiutility.h"
#include "logger.h"
#include "ui_logbrowser.h"

#include "resources/resources.h"

namespace OCC {

// ==============================================================================

LogBrowser::LogBrowser(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LogBrowser)
{
    ui->setupUi(this);

    ui->warningIcon->setPixmap(Resources::getCoreIcon(QStringLiteral("warning")).pixmap(ui->warningIcon->size()));
    ui->locationLabel->setText(Logger::instance()->temporaryFolderLogDirPath());

    ui->enableLoggingButton->setChecked(ConfigFile().automaticLogDir());
    connect(ui->enableLoggingButton, &QCheckBox::toggled, this, &LogBrowser::togglePermanentLogging);

    ui->httpLogButton->setChecked(ConfigFile().logHttp());
    connect(ui->httpLogButton, &QCheckBox::toggled, this, [](bool enable) {
        ConfigFile().configureHttpLogging(std::make_optional(enable));
    });

    ui->spinBox_numberOflogsToKeep->setValue(ConfigFile().automaticDeleteOldLogs());
    connect(ui->spinBox_numberOflogsToKeep, qOverload<int>(&QSpinBox::valueChanged), this, [](int i) {
        ConfigFile().setAutomaticDeleteOldLogs(i);
        Logger::instance()->setMaxLogFiles(i);
    });


    connect(ui->openFolderButton, &QPushButton::clicked, this, []() {
        QString path = Logger::instance()->temporaryFolderLogDirPath();
        QDir().mkpath(path);
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
    connect(ui->buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &QWidget::close);

    ConfigFile cfg;
    cfg.restoreGeometry(this);
}

LogBrowser::~LogBrowser()
{
}

void LogBrowser::setupLoggingFromConfig()
{
    ConfigFile config;
    auto logger = Logger::instance();

    if (config.automaticLogDir()) {
        // Don't override other configured logging
        if (logger->isLoggingToFile())
            return;

        logger->setupTemporaryFolderLogDir();
        Logger::instance()->setMaxLogFiles(config.automaticDeleteOldLogs());
    } else {
        logger->disableTemporaryFolderLogDir();
    }
}

void LogBrowser::togglePermanentLogging(bool enabled)
{
    ConfigFile config;
    config.setAutomaticLogDir(enabled);
    setupLoggingFromConfig();
}

} // namespace
