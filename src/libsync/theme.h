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

#include "common/utility.h"
#include "resources/resources.h"
#include "syncresult.h"

#include <QFileInfo>
#include <QObject>
#include <QPalette>
#include <qquickwindow.h>

namespace OCC {
class SyncResult;

/**
 * @brief The Theme class
 * @ingroup libsync
 */

class OWNCLOUDSYNC_EXPORT Theme : public QObject
{
    Q_OBJECT

public:
    enum class VersionFormat {
        Plain,
        Url,
        RichText,
        OneLiner
    };
    Q_ENUM(VersionFormat);

    /* returns a singleton instance. */
    static Theme *instance();
    static Theme *create(QQmlEngine *qmlEngine, QJSEngine *);

    ~Theme() override;

    /**
     * @brief appNameGUI - Human readable application name.
     *
     * Use and redefine this if the human-readable name contains spaces,
     * special chars and such.
     *
     * By default, the name is derived from the APPLICATION_NAME
     * cmake variable.
     *
     * @return QString with human readable app name.
     */
    virtual QString appNameGUI() const;

    /**
     * @brief appName - Application name (short)
     *
     * Use and redefine this as an application name. Keep it straight as
     * it is used for config files etc. If you need a more sophisticated
     * name in the GUI, redefine appNameGUI.
     *
     * By default, the name is derived from the APPLICATION_SHORTNAME
     * cmake variable, and should be the same. This method exists only for
     * legacy reasons.
     *
     * Warning: Do not modify this value, as many things, e.g. settings
     * depend on it! You most likely want to modify \ref appNameGUI().
     *
     * @return QString with app name.
     */
    QString appName() const;

    QString orgDomainName() const;

    QString vendor() const;

    /**
     * @brief configFileName
     * @return the name of the config file.
     */
    virtual QString configFileName() const;

    /**
     * get a sync state icon
     */

    QIcon themeTrayIcon(const SyncResult &result, bool sysTrayMenuVisible = false,
        Resources::IconType iconType = Resources::IconType::BrandedIconWithFallbackToVanillaIcon) const;

    QString syncStateIconName(const SyncResult &result) const;

    virtual QIcon applicationIcon() const;
    virtual QString applicationIconName() const;
    virtual QIcon aboutIcon() const;


    /**
    * URL to documentation.
    *
    * This is opened in the browser when the "Help" action is selected from the tray menu.
    *
    * If the function is overridden to return an empty string the action is removed from
    * the menu.
    *
    * Defaults to ownCloud's client documentation website.
    */
    virtual QString helpUrl() const;

    /**
     * The url to use for showing help on conflicts.
     *
     * If the function is overridden to return an empty string no help link will be shown.
     *
     * Defaults to helpUrl() + "conflicts.html", which is a page in ownCloud's client
     * documentation website. If helpUrl() is empty, this function will also return the
     * empty string.
     */
    virtual QString conflictHelpUrl() const;

    /**
     * Setting a value here will pre-define the server url.
     *
     * The respective UI controls will be disabled
     * NOT deprecated. overrideServerUrlV2 is gone as "overrides" (for what purpose? smells like a backdoor) should happen in theme only,
     * or via the managed device config
     */

    virtual QString overrideServerUrl() const;

    /**
     * If set to a non-empty string, the path part of the URL will be overwritten with this path.
     * This can be used to set the end-point to a fixed location, and thereby shorten the URL that
     * is given to the users.
     *
     * For example, if the URL for the product always contains `/dav` as the path, and setting that
     * here, and the URL given to the user is `example.com`, the branded application will contact
     * `example.com/dav`.
     */
    virtual QString overrideServerPath() const;


    /** @return color for the setup wizard. This is effectively the text color for the wizard pages*/
    virtual QColor wizardHeaderTitleColor() const;

    /** @return color for the setup wizard.  This is effectively the background color for each page*/
    virtual QColor wizardHeaderBackgroundColor() const;

    /** @return logo for the setup wizard. */
    virtual QIcon wizardHeaderLogo() const;

    /** @return logo that is used below the main wizard page content. */
    virtual QIcon wizardFooterLogo() const;

    /**
     * The used library versions
     */
    QString aboutVersions(VersionFormat format = VersionFormat::Plain) const;

    /**
     * About dialog contents
     */
    virtual QString about() const;

    /**
     * Define if the systray icons should be using mono design
     */
    void setSystrayUseMonoIcons(bool mono);


    /**
     * @brief Where to check for new Updates.
     */
    QUrl updateCheckUrl() const;


    /**
     * If this returns true, the user cannot configure the proxy in the network settings.
     * The proxy settings will be disabled in the configuration dialog.
     * Default returns false.
     */
    virtual bool forceSystemNetworkProxy() const;

    /**
     * @brief wizardUrlPlaceholder provides placeholder text for the URL field in the new account wizard
     * @return empty string unless overridden
     */
    virtual QString wizardUrlPlaceholder() const;

    /**
     * The OAuth client_id, secret pair.
     * Note that client that change these value cannot connect to un-branded owncloud servers.
     */
    virtual QString oauthClientId() const;
    virtual QString oauthClientSecret() const;

    /**
     * List of ports to use for the local redirect server
     */
    virtual QVector<quint16> oauthPorts() const;

    /**
     * Returns the required OpenIDConnect scopes
     */
    virtual QString openIdConnectScopes() const;

    /**
     * Returns the OpenIDConnect prompt type
     * It is supposed to be "consent select_account".
     * For "Konnect" it currently needs to be select_account,
     * which is the current default.
     */
    virtual QString openIdConnectPrompt() const;


    /**
     * On windows, force the user to use vfs, by disabling options to turn it off.
     * Default: false
     */
    virtual bool forceVirtualFilesOption() const;

    /**
     * Returns a list of IconName, Name, Url triplets that will be displayed as buttons or menu items in the main view
     * For each url an optional icon can be provided in the form of #IconName.svg or multiple #IconName-#resolution.png like for the other theme icons.
     * if the icon name is empty or "wrong" there will be no icon on the action, just text
     * */
    virtual QVector<std::tuple<QString, QString, QUrl>> urlActions() const;

    /**
     * Set the default value for move to trash option
     * Default: false
     */
    virtual bool moveToTrashDefaultValue() const;

    /**
     * @brief Allow the system configuration to override theme values.
     * @default false
     */
    virtual bool allowSystemConfigOverrides() const;

    /**
     * @brief Automatically add sync connections for newly discovered Spaces.
     *
     * Default: false
     * See #11749
     */
    virtual bool syncNewlyDiscoveredSpaces() const;

    /**
     * Whether to call spaces "Spaces" in the UI, or call them "Folders"
     *
     * Default: false
     */
    virtual bool spacesAreCalledFolders() const;

    bool withCrashReporter() const;

protected:
    Theme();

    /**
     * The SHA sum of the released git commit
     */
    QString gitSHA1(VersionFormat format = VersionFormat::Plain) const;


Q_SIGNALS:
    void systrayUseMonoIconsChanged(bool);


private:
    Theme(Theme const &);
    Theme &operator=(Theme const &);

    static Theme *_instance;
    bool _mono = false;
};

}
