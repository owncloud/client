/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "owncloudtheme.h"

#include <QCoreApplication>
#include <QIcon>
#include <QString>
#include <QVariant>

namespace OCC {

ownCloudTheme::ownCloudTheme()
    : Theme()
{
}

QColor ownCloudTheme::wizardHeaderBackgroundColor() const
{
    return QColor(4, 30, 66);
}

QColor ownCloudTheme::wizardHeaderTitleColor() const
{
    return Qt::white;
}

QIcon ownCloudTheme::wizardHeaderLogo() const
{
    return Resources::themeUniversalIcon(QStringLiteral("wizard_logo"));
}

QIcon ownCloudTheme::aboutIcon() const
{
    return Resources::themeUniversalIcon(QStringLiteral("oc-image-about"));
}

bool ownCloudTheme::moveToTrashDefaultValue() const
{
    // for the vanilla ownCloud client move-to-trash option is enabled by default
    return true;
}

bool ownCloudTheme::allowSystemConfigOverrides() const
{
    return true;
}
}
