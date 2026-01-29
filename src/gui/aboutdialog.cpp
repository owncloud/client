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
#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "gui/guiutility.h"
#include "libsync/theme.h"

#ifdef WITH_AUTO_UPDATER
#include "libsync/configfile.h"
#include "updater/ocupdater.h"
#ifdef Q_OS_MAC
// FIXME We should unify those, but Sparkle does everything behind the scene transparently
#include "updater/sparkleupdater.h"
#endif
#endif

namespace {
#ifdef WITH_AUTO_UPDATER
bool isTestPilotCloudTheme()
{
    return OCC::Theme::instance()->appName() == QLatin1String("testpilotcloud");
}
#endif
}

namespace OCC {

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->aboutText->setText(Theme::instance()->about());
    ui->icon->setPixmap(Theme::instance()->aboutIcon().pixmap(256));
    ui->versionInfo->setText(Theme::instance()->aboutVersions(Theme::VersionFormat::RichText));

    connect(ui->versionInfo, &QTextBrowser::anchorClicked, this, &AboutDialog::openBrowserFromUrl);
    connect(ui->aboutText, &QLabel::linkActivated, this, &AboutDialog::openBrowser);

    setupUpdaterWidget();
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::openBrowser(const QString &s)
{
    Utility::openBrowser(QUrl(s), this);
}

void AboutDialog::openBrowserFromUrl(const QUrl &s)
{
    return openBrowser(s.toString());
}

void AboutDialog::setupUpdaterWidget()
{
#ifdef WITH_AUTO_UPDATER
    // non-standard update channels are only supported by the vanilla theme and the testpilotcloud theme
    if (!Resources::isVanillaTheme() && !isTestPilotCloudTheme()) {
        if (Utility::isMac()) {
            // Because we don't have any statusString from the SparkleUpdater anyway we can hide the whole thing
            ui->updaterWidget->hide();
        }
    }

    if (!ConfigFile().skipUpdateCheck() && Updater::instance()) {
        // Note: the sparkle-updater is not an OCUpdater
        if (auto *ocupdater = qobject_cast<OCUpdater *>(Updater::instance())) {
            auto updateInfo = [ocupdater, this] {
                QString statusString = ocupdater->statusString();
                switch (ocupdater->downloadState()) {
                case OCUpdater::Unknown:
                    [[fallthrough]];
                case OCUpdater::CheckingServer:
                    [[fallthrough]];
                case OCUpdater::UpToDate:
                    // No update, leave the status string as is.
                    break;
                case OCUpdater::Downloading:
                    [[fallthrough]];
                case OCUpdater::DownloadComplete:
                    [[fallthrough]];
                case OCUpdater::DownloadFailed:
                    [[fallthrough]];
                case OCUpdater::DownloadTimedOut:
                    [[fallthrough]];
                case OCUpdater::UpdateOnlyAvailableThroughSystem:
                    statusString = QStringLiteral("Version %1 is available. %2").arg(ocupdater->availableVersionString(), statusString);
                    break;
                }

                ui->updateStateLabel->setText(statusString);
                ui->restartButton->setVisible(ocupdater->downloadState() == OCUpdater::DownloadComplete);
            };
            connect(ocupdater, &OCUpdater::downloadStateChanged, this, updateInfo);
            connect(ui->restartButton, &QAbstractButton::clicked, ocupdater, &Updater::applyUpdateAndRestart);
            updateInfo();
        }
#ifdef HAVE_SPARKLE
        if (SparkleUpdater *sparkleUpdater = qobject_cast<SparkleUpdater *>(Updater::instance())) {
            ui->updateStateLabel->setText(sparkleUpdater->statusString());
            ui->restartButton->setVisible(false);
        }
#endif
    } else {
        ui->updaterWidget->hide();
    }
#else
    ui->updaterWidget->hide();
#endif
}

} // OCC namespace
