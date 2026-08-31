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

#include <QObject>


// #include <QFileInfo>
#include <QColor>

#include "owncloudlib.h"

namespace OCC {

/**
 * @brief The Theme class allows branded clients to override certain values in the application.
 * @ingroup libsync
 */
class OWNCLOUDSYNC_EXPORT Theme : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief The VersionFormat enum controls how version information is formatted in the application.
     */
    enum class VersionFormat {
        Plain,
        Url,
        RichText,
        OneLiner
    };
    Q_ENUM(VersionFormat);

    /* polymorphically returns a singleton instance of the branding theme in play. */
    static Theme *instance();

    ~Theme() override;

    /**
     * @brief appNameGUI is the human readable application name.
     *
     * Override this if the gui should present a name which does not match the cmake definition,
     * eg. when the name should include spaces or other special characters.
     *
     * @default value of the APPLICATION_NAME cmake variable.
     *
     * @return QString for human readable app name.
     */
    virtual QString appNameGUI() const;

    /**
     * @brief appName is the general application name.
     *
     * @return value of the APPLICATION_SHORTNAME cmake variable.
     */
    QString appName() const;

    /**
     * @brief orgDomainName
     *
     * @return  the value of APPLICATION_REV_DOMAIN cmake variable.

     */
    QString orgDomainName() const;

    /**
     * @brief vendor
     *
     * @return value of the APPLICATION_VENDOR cmake variable.
     */
    QString vendor() const;

    /**
     * @brief configFileName is the name used for the application's user configuration file.
     *
     * @default value of cmake variable APPLICATION_EXECUTABLE + ".cfg".
     *
     * @return the name of the config file.
     */
    virtual QString configFileName() const;

    /**
     * @brief applicationIcon
     *
     * @default the default implementation expects that the universal folder contains an icon
     * named applicationIconName() + "-icon" (with extension of .svg or a collection of
     * one or more sized .png's)
     *
     * @return the icon for the application
     */
    // todo: dc-346 the icon retrieval functions must go
    virtual QIcon applicationIcon() const;

    /**
     * @brief applicationIconName
     *
     * @default value of cmake variable APPLICATION_SHORTNAME.
     *
     * @return the base application icon name.
     */
    virtual QString applicationIconName() const;

    /**
     * @brief aboutIcon
     *
     * @default returns applicationIcon().
     *
     * @return the icon to be used in the "about" gui.
     */
    virtual QIcon aboutIcon() const;


    /**
     * @brief helpUrl
     *
     * This is opened in the browser when the "Help" action is selected from the tray menu or
     * the "more" menu in the main window.
     *
     * If the function is overridden to return an empty string the action is removed from
     * the menu.
     *
     * @default ownCloud's client documentation URL.
     *
     * @return URL for help documentation.
     */
    virtual QString helpUrl() const;

    /**
     * @brief conflictHelpUrl
     *
     * The url to use for help on conflicts shown in the sync errors panel.
     *
     * If the function is overridden to return an empty string no help link for conflicts will be shown.
     *
     * @default helpUrl() + "conflicts.html".
     * Note that if the help URL is empty, the default implementation returns empty string.

     * @return the URL for conflicts help.
     */
    virtual QString conflictHelpUrl() const;

    /**
     * @brief overrideServerUrl
     *
     * Sets a predefined URL in the new account wizard.
     *
     * If this value is non-empty, the URL field in the account wizard will be disabled so the user must use
     * this URL to create accounts.
     *
     * @default empty string.
     *
     * @return the predefined URL for creating new accounts.
     */
    virtual QString overrideServerUrl() const;


    /**
     * @brief overrideServerPath
     *
     * If non-emtpy, overwrites the path segment of the account's URL with this path.
     *
     * This can be used to set the end-point to a fixed location, and thereby shorten the URL that
     * the user must provide when creating a new account.
     *
     * For example, if the URL for the product always contains `/dav` as the path, and the URL provided
     * by the user is `example.com`, the branded application will contact `example.com/dav`.
     *
     * @default empty string.
     *
     * @return the server path to be used with all account URLs.
     */
    virtual QString overrideServerPath() const;


    /** @return color for the setup wizard. This is effectively the text color for the wizard pages*/
    /**
     * @brief wizardHeaderTitleColor allows branding to override text color for wizard pages.
     *
     * If the value is empty, the default system text color will be used.
     *
     * @default empty color.
     *
     * @return the preferred text color for the wizards.
     */
    virtual QColor wizardHeaderTitleColor() const;

    /** @return color for the setup wizard.  This is effectively the background color for each page*/
    /**
     * @brief wizardHeaderBackgroundColor allows branding to override the background color of wizard pages
     *
     * If the value is empty, the default system background color will be used.
     *
     * @default empty color.
     *
     * @return the preferred background color for the wizard pages.
     */
    virtual QColor wizardHeaderBackgroundColor() const;

    /** @return logo for the setup wizard. */
    /**
     * @brief wizardHeaderLogo which appears in some wizard pages.
     *
     * @default is applicationIcon().
     *
     * @return the icon that should be used in the wizard.
     */
    virtual QIcon wizardHeaderLogo() const;

    /** @return logo that is used below the main wizard page content. */
    /**
     * @brief wizardFooterLogo appears below all other content in some wizard pages
     *
     * @default empty QIcon.
     *
     * @return the footer logo to be used in the wizard.
     */
    virtual QIcon wizardFooterLogo() const;


    /** @brief aboutVersions supplies the version information including the current application version as well
     * as versions of various build dependencies.
     *
     * @param format determines how the version information is formatted.
     *      VersionFormat::OneLiner is appropriate for log output
     *      VersionFormat::RichText is appropriate for various gui's, but importantly, it provides a url link to the build version's sha in github.
     *      VersionFormat::Url returns build version text with a url link to the sha in github.
     *      VersionFormat::Plain is what you'd expect: it's just the version information with plain text formatting.
     *
     * @return the formatted version information for the current build.
     */
    QString aboutVersions(VersionFormat format = VersionFormat::Plain) const;


    /**
     * @brief about defines the text used in the About panel.
     *
     * @default the About text for ownCloud.
     *
     * @return the About text.
     */
    virtual QString about() const;


    /**
     * @brief updateCheckUrl
     *
     * This value controls the location used for update checks, and effectively controls whether the updater feature is enabled.
     *
     * The URL is defined by the cmake environment variable APPLICATION_UPDATE_URL. If this variable
     * does not exist, an empty URL is returned.
     *
     * @return the value of APPLICATION_UPDATE_URL (may be empty).
     */
    QUrl updateCheckUrl() const;

    /**
     * @brief forceSystemNetworkProxy
     *
     * If this returns true, the user cannot configure the network proxy in the Settings panel.
     *
     * @default false.
     *
     * @return whether the system proxy should always be used.
     */
    virtual bool forceSystemNetworkProxy() const;

    /**
     * @brief wizardUrlPlaceholder provides placeholder text for the URL field in the new account wizard.
     *
     * @default empty string.
     *
     * @return QString with URL placeholder.
     */
    virtual QString wizardUrlPlaceholder() const;

    /**
     * The following five functions exist to define the client's OIDC configuration.
     *
     * At minimum the ClientId and ClientSecret should be overridden for all branded clients.
     *
     */

    /**
     * @brief oauthClientId
     *
     * @default the OIDC client Id for ownCloud servers.
     *
     * @return the OIDC client Id.
     */
    virtual QString oauthClientId() const;

    /**
     * @brief oauthClientSecret
     *
     * @default the OIDC client secret for ownCloud servers.
     *
     * @return the OIDC client secret.
     */
    virtual QString oauthClientSecret() const;


    /**
     * @brief oauthPorts
     *
     * The list of ports to use for the local redirect server.
     *
     * @default is 0, which means any port can be used.
     *
     * @return the list of allowed OIDC ports.
     */
    virtual QVector<quint16> oauthPorts() const;

    /**
     * @brief openIdConnectScopes
     *
     * @default "openid offline_access email profile"
     *
     * @return the allowed OIDC scopes.
     */
    virtual QString openIdConnectScopes() const;

    /**
     * @brief openIdConnectPrompt
     *
     * @default "select_account consent"
     *
     * @return the OIDC connect prompt.
     */
    virtual QString openIdConnectPrompt() const;


    /**
     * @brief forceVirtualFilesOption
     *
     * When this returns true, the user must use VFS on Windows (and any other platforms that have
     * a VFS implementation).
     *
     * @default false.
     *
     * @return whether VFS should be forced on in the application.
     */
    virtual bool forceVirtualFilesOption() const;

    /**
     * @brief urlActions
     *
     * Defines a list of IconName, Text, Url triplets that will be displayed as buttons or menu items in the main view.
     *
     * For each url an optional icon name can be provided.
     * If the icon name is empty or can't be located in resources, there will be no icon on the action, just text.
     *
     * @default empty.
     *
     * @return definitions for URL actions.
     */
    virtual QVector<std::tuple<QString, QString, QUrl>> urlActions() const;

    /**
     * @brief moveToTrashDefaultValue sets a default value for move-to-trash option in the Settings panel.
     *
     * @default false.
     *
     * @return the default value for move-to-trash in the Settings.
     */
    virtual bool moveToTrashDefaultValue() const;

    /**
     * @brief Allow a system configuration to override theme values related to the OIDC parameters.
     *
     * @default false.
     *
     * @return whether a system configuration can be used to replace the theme's OIDC values.
     */
    virtual bool allowSystemConfigOverrides() const;

    /**
     * @brief syncNewlyDiscoveredSpaces
     *
     * Automatically add sync connections for newly discovered Spaces.
     *
     * Note this functionality is implemented but may not be very nice for users!
     *
     * If this functionality is desired please contact dev to discuss the fine points, and which updates
     * may be useful to make the experience more pleasant.
     *
     * @default false.
     *
     * @return whether to automatically sync new spaces from the server.
     */
    virtual bool syncNewlyDiscoveredSpaces() const;

    /**
     * @brief spacesAreCalledFolders determines whether the gui should call spaces "Folders" or not.
     *
     * @default false.
     *
     * @return call spaces "Folders".
     */
    virtual bool spacesAreCalledFolders() const;

    /**
     * @brief withCrashReporter reveals whether crash reporting should be enabled.
     *
     * @return value for cmake variable WITH_CRASHREPORTER.
     */
    bool withCrashReporter() const;

protected:
    Theme();

    /**
     * The SHA sum of the released git commit
     */
    QString gitSHA1(VersionFormat format = VersionFormat::Plain) const;


private:
    Theme(Theme const &);
    Theme &operator=(Theme const &);

    static Theme *_instance;
};

}
