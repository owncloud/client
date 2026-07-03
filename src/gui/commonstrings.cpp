/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
