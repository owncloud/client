/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
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

#pragma once

#include "owncloudlib.h"

#include <QNetworkProxy>
#include <QSettings>
#include <QSharedPointer>
#include <QString>
#include <QVariant>

#include <chrono>
#include <memory>
#include <optional>

class QWidget;
class QHeaderView;
class ExcludedFiles;

namespace OCC {

class AbstractCredentials;

/**
 * @brief The ConfigFile class
 * @ingroup libsync
 */
class OWNCLOUDSYNC_EXPORT ConfigFile
{
public:
    static QString configPath();
    static QString configFile();
    static QSettings makeQSettings();
    static bool exists();

    ConfigFile();

    enum Scope {
        UserScope,
        SystemScope
    };

    QString excludeFile(Scope scope) const;
    static QString excludeFileFromSystem(); // doesn't access config dir

    /**
     * Creates a backup of the file
     *
     * Returns the path of the new backup.
     */
    QString backup() const;


    QString defaultConnection() const;

    /* Server poll interval in milliseconds */
    std::chrono::milliseconds remotePollInterval(std::chrono::seconds defaultVal, const QString &connection = QString()) const;

    /**
     * Interval in milliseconds within which full local discovery is required
     *
     * Use -1 to disable regular full local discoveries.
     */
    // todo: there is no setter for this so it needs to go. the default value returned is 1h so move it somehwere that it makes more sense
    // note a very suspicious split use of this value in both Folder::startSync AND SyncScheduler::SyncScheduler (where it sets the repeat interval
    // on a timer)
    std::chrono::milliseconds fullLocalDiscoveryInterval() const;

    bool monoIcons() const;
    void setMonoIcons(bool);

    bool promptDeleteFiles() const;
    void setPromptDeleteFiles(bool promptDeleteFiles);

    bool crashReporter() const;
    void setCrashReporter(bool enabled);

    /** Whether to set up logging to a temp directory on startup.
     *
     * Configured via the log window. Not used if command line sets up logging.
     */
    bool automaticLogDir() const;
    void setAutomaticLogDir(bool enabled);

    /** Number of log files to keep */
    int automaticDeleteOldLogs() const;
    void setAutomaticDeleteOldLogs(int number);

    /** Whether to log http traffic */
    bool logHttp() const;

    /**
     * Set up HTTP logging.
     * This method should be called during application startup to make sure no messages are missed.
     */
    void configureHttpLogging(std::optional<bool> enable = std::nullopt);

    // proxy settings
    void setProxyType(
        QNetworkProxy::ProxyType proxyType, const QString &host = QString(), int port = 0, bool needsAuth = false, const QString &user = QString());

    int proxyType() const;
    QString proxyHostName() const;
    int proxyPort() const;
    bool proxyNeedsAuth() const;
    QString proxyUser() const;

    bool pauseSyncWhenMetered() const;
    void setPauseSyncWhenMetered(bool isChecked);

    /// Used for testing, so we do not change the user's config file.
    // this is *only* used for testing. Try to do something here as generally we should not allow changing the config location
    // by any rando via a public interface.
    static bool setConfDir(const QString &value);

    bool optionalDesktopNotifications() const;
    void setOptionalDesktopNotifications(bool show);

    std::optional<QStringList> issuesWidgetFilter() const;
    void setIssuesWidgetFilter(const QStringList &checked);

    // no setter, and the default return value is 5 minutes. This already matches the AbstractNetworkJob::DefaultHttpTimeout
    // it's completely pointless so needs to die!
    // note the only possible way to change this (currently) is with env var OWNCLOUD_TIMEOUT which should be int value to represent seconds
    std::chrono::seconds timeout() const;

    // todo: could be moved to main window or main window controller
    void saveGeometry(QWidget *w);
    void restoreGeometry(QWidget *w);

    // todo: these could be moved to the header impl (or related controller)
    void saveGeometryHeader(QHeaderView *header);
    bool restoreGeometryHeader(QHeaderView *header);

    // how often the check about new versions runs
    // todo: move this. No setter means it's not actually in the config file :/
    std::chrono::milliseconds updateCheckInterval(const QString &connection = QString()) const;

    QString uiLanguage() const;
    void setUiLanguage(const QString &uiLanguage);

    /** The client version that last used this settings file.
        Updated by configVersionMigration() at client startup. */
    QString clientVersionWithBuildNumberString() const;
    void setClientVersionWithBuildNumberString(const QString &version);

    /**  Returns a new settings pre-set in a specific group. */
    // todo: having a pointer for this is questionable.
    // having this at all is questionable. Generally, the callers hold the group name they want to use, so it's more
    // reasonable to have them get the settings then set the group as needed. It's already convenient enough that having a
    // convenience function is overkill
    static std::unique_ptr<QSettings> settingsWithGroup(const QString &group);

    /// Add the system and user exclude file path to the ExcludedFiles instance.
    // todo: move this - no setter so does not particularly belong in settings.
    static void setupDefaultExcludeFilePaths(ExcludedFiles &excludedFiles);

private:
    QVariant getValue(const QString &param, const QString &group = QString(),
        const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);

private:
    // static QString _oCVersion;
    static QString _confDir;
};
}
