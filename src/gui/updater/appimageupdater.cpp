/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <appimage/update.h>
#include <chrono>

#include "appimageupdater.h"
#include "common/version.h"
#include "libsync/configfile.h"
#include "mainwindow/modalwrapperwidget.h"
#include "theme.h"
#include "updater_private.h"

#include "appimageupdateavailablewidget.h"
#include "application.h"

using namespace OCC;
using namespace std::chrono_literals;

namespace {

/**
 * libappimageupdate uses exceptions, but the client does not
 * This little shim adapts the interface to one usable within this project
 */
class AppImageUpdaterShim : public QObject
{
    Q_OBJECT

private:
    explicit AppImageUpdaterShim(const QString &zsyncFileUrl, QObject *parent = nullptr)
        : QObject(parent)
        , _updater(Utility::appImageLocation().toStdString(), true)
    {
        QString updateInformation(QStringLiteral("zsync|") + zsyncFileUrl);
        _updater.setUpdateInformation(updateInformation.toStdString());
    }

    void _logStatusMessages()
    {
        std::string currentStatusMessage;

        while (_updater.nextStatusMessage(currentStatusMessage)) {
            qCInfo(lcUpdater) << "AppImageUpdate:" << QString::fromStdString(currentStatusMessage);
        }
    }

public:
    static AppImageUpdaterShim *makeInstance(const QString &updateInformation, QObject *parent)
    {
        try {
            return new AppImageUpdaterShim(updateInformation, parent);
        } catch (const std::exception &e) {
            qCCritical(lcUpdater) << "Failed to create updater shim:" << e.what();
            return nullptr;
        }
    }

    bool isUpdateAvailable() noexcept
    {
        try {
            bool updateAvailable;

            if (!_updater.checkForChanges(updateAvailable)) {
                _logStatusMessages();
                return false;
            }

            _logStatusMessages();
            return updateAvailable;
        } catch (const std::exception &e) {
            _logStatusMessages();
            qCCritical(lcUpdater) << "Checking for update failed:" << e.what();
            return false;
        }
    }

    void startUpdateInBackground() noexcept
    {
        // monitor progress and log status messages
        auto *timer = new QTimer(this);

        timer->setInterval(100ms);

        connect(timer, &QTimer::timeout, this, [=]() {
            _logStatusMessages();

            if (_updater.isDone()) {
                Q_EMIT finished(!_updater.hasError());
                timer->stop();
            }
        });

        _updater.start();
        timer->start();
    }

Q_SIGNALS:
    void finished(bool successfully);

private:
    appimage::update::Updater _updater;
};

} // namespace

AppImageUpdater::AppImageUpdater(const QUrl &url)
    : OCUpdater(url)
{
}

void AppImageUpdater::versionInfoArrived(const UpdateInfo &info)
{
    const auto &currentVersion = Version::versionWithBuildNumber();
    const auto newVersion = QVersionNumber::fromString(info.version());

    if (info.version().isEmpty() || currentVersion >= newVersion) {
        qCInfo(lcUpdater) << "Client is on latest version!";
        setDownloadState(UpToDate);
        return;
    }

    const auto previouslySkippedVersion = this->previouslySkippedVersion();
    if (previouslySkippedVersion >= newVersion) {
        qCInfo(lcUpdater) << "Update" << previouslySkippedVersion << "was skipped previously by user";
        setDownloadState(UpToDate);
        return;
    }

    const auto appImageUpdaterShim = AppImageUpdaterShim::makeInstance(info.downloadUrl(), this);

    if (appImageUpdaterShim == nullptr) {
        setDownloadState(DownloadFailed);
        return;
    }

    if (!appImageUpdaterShim->isUpdateAvailable()) {
        qCCritical(lcUpdater) << "Update server reported that update is available, but AppImageUpdate disagrees, aborting";
        setDownloadState(DownloadFailed);
        return;
    }

    auto widget = new AppImageUpdateAvailableWidget(currentVersion, newVersion, ocApp()->mainWindow());

    connect(widget, &AppImageUpdateAvailableWidget::skipUpdateButtonClicked, this, [newVersion]() {
        qCInfo(lcUpdater) << "Update" << newVersion << "skipped by user";
        setPreviouslySkippedVersion(newVersion);
    });

    connect(widget, &AppImageUpdateAvailableWidget::accepted, this, [this, widget, appImageUpdaterShim]() {
        // binding AppImageUpdaterShim shared pointer to finished callback makes sure the updater is cleaned up when it's done
        connect(appImageUpdaterShim, &AppImageUpdaterShim::finished, this, [this](bool succeeded) {
            if (succeeded) {
                qCInfo(lcUpdater) << "AppImage update complete";
                setDownloadState(DownloadComplete);
            } else {
                qCInfo(lcUpdater) << "AppImage update failed";
                setDownloadState(DownloadFailed);
            }
        });

        setDownloadState(Downloading);
        appImageUpdaterShim->startUpdateInBackground();
    });

    ModalWrapperWidget *wrapper = new ModalWrapperWidget(widget, ocApp()->mainWindow());
    ocApp()->showModalWidget(wrapper);
}

void AppImageUpdater::backgroundCheckForUpdate()
{
    OCUpdater::backgroundCheckForUpdate();
}

#include "appimageupdater.moc"
