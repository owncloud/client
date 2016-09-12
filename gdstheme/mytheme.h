/*
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>, ownCloud Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#ifndef MY_THEME_H
#define MY_THEME_H

#include "theme.h"

#include <QString>
#include <QDebug>
#include <QPixmap>
#include <QIcon>
#include <QApplication>


namespace OCC {

class MyTheme : public Theme
{
public:
    MyTheme() : Theme() {}

    QIcon folderDisabledIcon() const Q_DECL_OVERRIDE { return themeIcon( QLatin1String("state-error" ) ); }
    QString configFileName() const Q_DECL_OVERRIDE  { return QLatin1String( "GreenDataStorage.cfg" ); }

    /*
     * If you changed file names in /replacements/mytheme/theme/colored
     * you will have to do that here also.
     */
    QIcon applicationIcon() const Q_DECL_OVERRIDE {
        return themeIcon( QLatin1String( "GreenDataStorage-icon" ) );
    }

    /*
     * If you have your own help page change the URL below.
     * example: return QLatin1String( "http://www.yourwebsite.com/support" );
     */
    virtual QString helpUrl() const {
        return QString::fromLatin1("http://doc.owncloud.org/desktop/%1.%2/").arg(MIRALL_VERSION_MAJOR).arg(MIRALL_VERSION_MINOR);
    }

    /*
     * If you have your own URL for updates, change it below.
     * example: return QLatin1String( "http://www.yourwebsite.com/updates/" );
     */
    virtual QString updateCheckUrl() const {
        return QLatin1String( "http://deploy.fjit.no/GreenDataStorage/client/" );
    }

    /*
     * Enter the color of the header title in the setup wizard.
     * example: return QColor("#FFFFFF");
     */
	virtual QColor wizardHeaderTitleColor() const
	{
	    return QColor("#D36D2F");
	}

    /*
     * Enter the background color of the header in the setup wizard.
     * example: return QColor("#10283F");
     */
	virtual QColor wizardHeaderBackgroundColor() const
	{
	    return QColor("#000000");
	}

    /*
     * The logo you want to be seen in the header of the setup wizard..
     * example: return QPixmap(":/mirall/theme/colored/wizard_logo.png");
     */
	virtual QPixmap wizardHeaderLogo() const
	{
	    return QPixmap(":/gdstheme/theme/colored/wizard_logo.png");
	}


	virtual QString about() const
	{
	    QString re;
	    re = tr("<p>Version %1. For more information please visit <a href='%2'>%3</a>.</p>")
	            .arg(MIRALL_VERSION_STRING).arg("http://" MIRALL_STRINGIFY(APPLICATION_DOMAIN))
	            .arg(MIRALL_STRINGIFY(APPLICATION_DOMAIN));

	    re += tr("<p>Copyright ownCloud, Incorporated</p>");
	    re += tr("<p>Distributed by %1 and licensed under the GNU General Public License (GPL) Version 2.0.<br/>"
	             "Copyrights to the %2 logo belongs to %1.</p>")
	            .arg(APPLICATION_VENDOR).arg(APPLICATION_NAME);

	    re += gitSHA1();
	    return re;
	}

	QString overrideServerUrl() const
	{
	    return QLatin1String("https://cloud.green-data-storage.com");
	}


};

} // namespace mirall

#endif // MY_THEME_H
