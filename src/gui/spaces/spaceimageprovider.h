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

#pragma once

#include "spacesmanager.h"
#include <QPointer>

#include <QQuickImageProvider>

namespace OCC::Spaces {
class SpaceImageProvider : public QQuickImageProvider
{
    Q_OBJECT
public:
    SpaceImageProvider(GraphApi::SpacesManager *spacesMgr);
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;


private:
    QPointer<GraphApi::SpacesManager> _spacesManager;
};

}
