/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "spaceimageprovider.h"
#include "libsync/graphapi/spacesmanager.h"
#include "resources/qmlresources.h"
#include "resources/resources.h"

using namespace OCC;
using namespace Spaces;

SpaceImageProvider::SpaceImageProvider(GraphApi::SpacesManager *spacesMgr)
    : QQuickImageProvider(QQuickImageProvider::Pixmap, QQuickImageProvider::ForceAsynchronousImageLoading)
    , _spacesManager(spacesMgr)
{
}

QPixmap SpaceImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    if (!_spacesManager)
        return {};

    QIcon icon;
    if (id == QLatin1String("placeholder")) {
        icon = Resources::getCoreIcon(QStringLiteral("defaultSpaceImage"));
    } else {
        const auto ids = id.split(QLatin1Char('/'));
        const auto *space = _spacesManager->space(ids.last());
        if (space) {
            icon = space->image()->image();
        }
    }
    Q_ASSERT(!icon.isNull());
    return Resources::pixmap(requestedSize, icon, QIcon::Normal, size);
}
