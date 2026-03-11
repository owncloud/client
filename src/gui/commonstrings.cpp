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
#include "commonstrings.h"

#include <QCoreApplication>
#include <theme.h>

using namespace OCC;

QString CommonStrings::fileBrowser()
{
#ifdef Q_OS_WIN
    return QStringLiteral("Explorer");
#elif defined(Q_OS_MAC)
    return QStringLiteral("Finder");
#else
    return QCoreApplication::translate("CommonStrings", "file manager");
#endif
}

QString CommonStrings::showInFileBrowser()
{
    return QCoreApplication::translate("CommonStrings", "Show in %1").arg(fileBrowser());
}

QString CommonStrings::showInWebBrowser()
{
    return QCoreApplication::translate("CommonStrings", "Show in web browser");
}

QString CommonStrings::copyToClipBoard()
{
    return QCoreApplication::translate("CommonStrings", "Copy");
}

QString CommonStrings::filterButtonText(int filterCount)
{
    return QCoreApplication::translate("CommonStrings", "%n Filter(s)", nullptr, filterCount);
}

QString CommonStrings::space()
{
    if (Theme::instance()->spacesAreCalledFolders())
        return QCoreApplication::translate("CommonStrings", "folder");
    else
        return QCoreApplication::translate("CommonStrings", "space");
}

QString CommonStrings::spaces()
{
    if (Theme::instance()->spacesAreCalledFolders())
        return QCoreApplication::translate("CommonStrings", "folders");
    else
        return QCoreApplication::translate("CommonStrings", "spaces");
}

QString CommonStrings::capSpace()
{
    if (Theme::instance()->spacesAreCalledFolders())
        return QCoreApplication::translate("CommonStrings", "Folder");
    else
        return QCoreApplication::translate("CommonStrings", "Space");
}
