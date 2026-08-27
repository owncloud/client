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

#include "theme.h"
#include "common/utility.h"
#include "common/version.h"
#include "common/vfs.h"
#include "config.h"

#include "resources/iconresources.h"
#include "resources/resources.h"

#include <QSslSocket>
#include <QStyle>

#ifdef THEME_INCLUDE
#include THEME_INCLUDE
#endif

namespace OCC {

Theme *Theme::_instance = nullptr;

Theme *Theme::instance()
{
    if (!_instance) {
        _instance = new THEME_CLASS;
        //      auto *watcher = new Resources::ThemeWatcher(_instance);
        //      connect(watcher, &Resources::ThemeWatcher::themeChanged, _instance, &Theme::themeChanged);
    }
    return _instance;
}

Theme *Theme::create(QQmlEngine *qmlEngine, QJSEngine *)
{
    Q_ASSERT(qmlEngine->thread() == Theme::instance()->thread());
    QJSEngine::setObjectOwnership(Theme::instance(), QJSEngine::CppOwnership);
    return instance();
}

Theme::~Theme() { }

QString Theme::appNameGUI() const
{
    return QStringLiteral(APPLICATION_NAME);
}

QString Theme::appName() const
{
    return QStringLiteral(APPLICATION_SHORTNAME);
}

QString Theme::orgDomainName() const
{
    return QStringLiteral(APPLICATION_REV_DOMAIN);
}

QString Theme::vendor() const
{
    return QStringLiteral(APPLICATION_VENDOR);
}

QString Theme::configFileName() const
{
    return QStringLiteral(APPLICATION_EXECUTABLE ".cfg");
}

// returning an icon instead of just the icon name is really questionable here. Need to see if this is somehow required for the
// branding builds - if not, get rid of this entirely and make the applicationIconName return the correct name by appending the -icon
// this is SO confusing - theme should have no responsibility for *retrieving* resources!!!
QIcon Theme::applicationIcon() const
{
    return IconResources::getUniversalIcon(applicationIconName() + QStringLiteral("-icon"));
    // return Resources::themeUniversalIcon(applicationIconName() + QStringLiteral("-icon"));
}

QString Theme::applicationIconName() const
{
    // attention! if APPLICATION_ICON_NAME is not defined, it is auto-set to APPLICATION_SHORTNAME in CMakeLists.txt:131
    // I STRONGLY prefer to move that "fallback" here, to avoid confusion, as *how many cmake files do we have to navigate to
    // find these values*?
    // it's not ok, imo. I quasi understand the "need" for these vars but they are nothing but trouble.
    return QStringLiteral(APPLICATION_ICON_NAME);
}

QIcon Theme::aboutIcon() const
{
    return applicationIcon();
}

Theme::Theme()
    : QObject(nullptr)
{
}

QString Theme::helpUrl() const
{
    return QStringLiteral("https://doc.owncloud.com/desktop/latest/");
}

QString Theme::conflictHelpUrl() const
{
    auto baseUrl = helpUrl();
    if (baseUrl.isEmpty())
        return QString();
    if (!baseUrl.endsWith(QLatin1Char('/')))
        baseUrl.append(QLatin1Char('/'));
    return baseUrl + QStringLiteral("conflicts.html");
}

QString Theme::overrideServerUrl() const
{
    return QString();
}

QString Theme::overrideServerPath() const
{
    return {};
}

QUrl Theme::updateCheckUrl() const
{
#ifndef APPLICATION_UPDATE_URL
    return QUrl(QString());
#else
    return QUrl(QStringLiteral(APPLICATION_UPDATE_URL));
#endif
}

QString Theme::gitSHA1(VersionFormat format) const
{
    const QString gitShahSort = Version::gitSha().left(6);
    const auto gitUrl = QStringLiteral("https://github.com/owncloud/client/commit/%1").arg(Version::gitSha());
    switch (format) {
    case Theme::VersionFormat::OneLiner:
        Q_FALLTHROUGH();
    case Theme::VersionFormat::Plain:
        return gitShahSort;
    case Theme::VersionFormat::Url:
        return gitUrl;
    case Theme::VersionFormat::RichText:
        return QStringLiteral("<a href=\"%1\">%3</a>").arg(gitUrl, gitShahSort);
    }
    return QString();
}

QString Theme::aboutVersions(Theme::VersionFormat format) const
{
    const QString br = [&format] {
        switch (format) {
        case Theme::VersionFormat::RichText:
            return QStringLiteral("<br>");
        case Theme::VersionFormat::Url:
            Q_FALLTHROUGH();
        case Theme::VersionFormat::Plain:
            return QStringLiteral("\n");
        case Theme::VersionFormat::OneLiner:
            return QStringLiteral(" ");
        }
        Q_UNREACHABLE();
    }();
    const QString qtVersion = QString::fromUtf8(qVersion());
    const QString qtVersionString = (QLatin1String(QT_VERSION_STR) == qtVersion
            ? qtVersion
            : QCoreApplication::translate("ownCloudTheme::qtVer", "%1 (Built against Qt %2)").arg(qtVersion, QStringLiteral(QT_VERSION_STR)));
    QString _version = Version::displayString();
    QString gitUrl;
    // just use the version::gitSha if (!Version::gitSha().isEmpty())
    {
        if (format != Theme::VersionFormat::Url) {
            _version = QCoreApplication::translate("ownCloudTheme::versionWithSha", "%1 %2").arg(_version, gitSHA1(format));
        } else {
            gitUrl = gitSHA1(format) + br;
        }
    }
    QStringList sysInfo = {QStringLiteral("OS: %1-%2 (build arch: %3, CPU arch: %4)")
            .arg(QSysInfo::productType(), QSysInfo::kernelVersion(), QSysInfo::buildCpuArchitecture(), Utility::currentCpuArch())};
    // may be called by both GUI and CLI, but we can display QPA only for the former
    if (auto guiApp = qobject_cast<QGuiApplication *>(qApp)) {
        sysInfo << QStringLiteral("QPA: %1").arg(guiApp->platformName());
    }

    return QCoreApplication::translate("ownCloudTheme::aboutVersions()",
        "%1 %2%7"
        "%8"
        "Libraries Qt %3, %4%7"
        "Using virtual files plugin: %5%7"
        "%6")
        .arg(appName(), _version, qtVersionString, QSslSocket::sslLibraryVersionString(),
            Utility::enumToString(VfsPluginManager::instance().bestAvailableVfsMode()), sysInfo.join(br), br, gitUrl);
}


QString Theme::about() const
{
    // Ideally, the vendor should be "ownCloud GmbH", but it cannot be changed without
    // changing the location of the settings and other registry keys.
    // todo: I do not agree with checking the resources for whether this is oc or not. imo we should just take the application_vendor in all
    // cases for this default about() impl.
    bool isOwnCloud = std::string_view(APPLICATION_SHORTNAME) == "ownCloud";
    const QString vendor = isOwnCloud ? QStringLiteral("ownCloud GmbH") : QStringLiteral(APPLICATION_VENDOR);
    return tr("<p>Version %1. For more information visit <a href=\"%2\">https://%3</a></p>"
              "<p>For known issues and help, please visit: <a href=\"https://central.owncloud.com/c/desktop-client\">https://central.owncloud.com</a></p>"
              "<p><small>By Klaas Freitag, Daniel Molkentin, Olivier Goffart, Markus Götz, "
              " Jan-Christoph Borchardt, Thomas Müller,<br>"
              "Dominik Schmidt, Michael Stingl, Hannah von Reth, Fabian Müller and others.</small></p>"
              "<p>Copyright ownCloud GmbH (A Kiteworks Company)</p>"
              "<p>Distributed by %4 and licensed under the GNU General Public License (GPL) Version 2.0.<br/>"
              "%5 and the %5 logo are registered trademarks of %4 in the "
              "United States, other countries, or both.</p>"
              "<p><small>%6</small></p>")
        .arg(Utility::escape(Version::displayString()), Utility::escape(QStringLiteral("https://" APPLICATION_DOMAIN)),
            Utility::escape(QStringLiteral(APPLICATION_DOMAIN)), Utility::escape(vendor), Utility::escape(appNameGUI()),
            aboutVersions(Theme::VersionFormat::RichText));
}

QString Theme::syncStateIconName(const SyncResult &result) const
{
    switch (result.status()) {
    case SyncResult::NotYetStarted:
        [[fallthrough]];
    case SyncResult::SyncRunning:
        return QStringLiteral("sync");
    case SyncResult::SyncAbortRequested:
        [[fallthrough]];
    case SyncResult::Paused:
        return QStringLiteral("pause");
    case SyncResult::SyncPrepare:
        [[fallthrough]];
    case SyncResult::Success:
        if (!result.hasUnresolvedConflicts()) {
            return QStringLiteral("ok");
        }
        [[fallthrough]];
    case SyncResult::Problem:
        [[fallthrough]];
    case SyncResult::Undefined:
        // this can happen if no sync connections are configured.
        return QStringLiteral("information");
    case SyncResult::Offline:
        return QStringLiteral("offline");
    case SyncResult::Error:
        [[fallthrough]];
    case SyncResult::Unavailable:
        [[fallthrough]];
    case SyncResult::SetupError:
        // FIXME: Use problem once we have an icon.
        return QStringLiteral("error");
    }
    Q_UNREACHABLE();
}

QColor Theme::wizardHeaderTitleColor() const
{
    return qApp->palette().text().color();
}

QColor Theme::wizardHeaderBackgroundColor() const
{
    return QColor();
}

QIcon Theme::wizardHeaderLogo() const
{
    return applicationIcon();
}

QIcon Theme::wizardFooterLogo() const
{
    return QIcon();
}

bool Theme::forceSystemNetworkProxy() const
{
    return false;
}

QString Theme::oauthClientId() const
{
    return QStringLiteral("xdXOt13JKxym1B1QcEncf2XDkLAexMBFwiT9j6EfhhHFJhs2KM9jbjTmf8JBXE69");
}

QString Theme::oauthClientSecret() const
{
    return QStringLiteral("UBntmLjC2yYCeHwsyj73Uwo9TAaecAetRwMw0xYcvNL9yRdLSUi0hUAHfvCHFeFh");
}

QVector<quint16> Theme::oauthPorts() const
{
    // zero means a random port
    return {0};
}

QString Theme::openIdConnectScopes() const
{
    return QStringLiteral("openid offline_access email profile");
}

QString Theme::openIdConnectPrompt() const
{
    return QStringLiteral("select_account consent");
}

QString Theme::wizardUrlPlaceholder() const
{
    return QString();
}

bool Theme::forceVirtualFilesOption() const
{
    return false;
}

QVector<std::tuple<QString, QString, QUrl>> Theme::urlActions() const
{
    return {};
}

bool Theme::moveToTrashDefaultValue() const
{
    return false;
}

bool Theme::allowSystemConfigOverrides() const
{
    return false;
}

bool Theme::syncNewlyDiscoveredSpaces() const
{
    return false;
}

bool Theme::spacesAreCalledFolders() const
{
    return false;
}

bool Theme::withCrashReporter() const
{
#ifdef WITH_CRASHREPORTER
    return true;
#else
    return false;
#endif
}

} // end namespace client
