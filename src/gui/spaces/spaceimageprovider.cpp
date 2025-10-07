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
        icon = Resources::getCoreIcon(QStringLiteral("space"));
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
