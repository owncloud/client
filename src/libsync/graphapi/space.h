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

#include "owncloudlib.h"

#include "libsync/accountfwd.h"

#include <OAIDrive.h>

#include <QIcon>
#include <QtQmlIntegration>

namespace OCC {
namespace GraphApi {
    class SpacesManager;
    class Space;

    class OWNCLOUDSYNC_EXPORT SpaceImage : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QUrl qmlImageUrl READ qmlImageUrl NOTIFY imageChanged)
        QML_ELEMENT
        QML_UNCREATABLE("C++ only")
    public:
        SpaceImage(Space *space);

        [[nodiscard]] QUrl url() const { return _url; }
        [[nodiscard]] QString etag() const { return _etag; }
        [[nodiscard]] QIcon image() const;

        [[nodiscard]] QUrl qmlImageUrl() const;

    Q_SIGNALS:
        void imageChanged();

    private:
        void update();

        QUrl _url;
        QString _etag;
        QIcon _image;
        Space *_space = nullptr;

        friend class Space;
    };

    class OWNCLOUDSYNC_EXPORT Space : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(SpaceImage *image READ image NOTIFY imageChanged)
        QML_ELEMENT
        QML_UNCREATABLE("Spaces can only be created by the SpacesManager")
    public:
        /***
         * Returns the display name of the drive.
         * This is identical to drive.getName() for most drives.
         * Exceptions: Personal spaces
         */
        QString displayName() const;

        QString id() const;


        OpenAPI::OAIDrive drive() const;

        /***
         * Assign a priority to a drive, used for sorting
         */
        uint32_t priority() const;

        /**
         * Whether a drive object has been deleted.
         */
        bool disabled() const;

        SpaceImage *image() const;

        QUrl webdavUrl() const;

    Q_SIGNALS:
        void imageChanged();

    private:
        Space(SpacesManager *spaceManager, const OpenAPI::OAIDrive &drive, bool hasManyPersonalSpaces);
        void setDrive(const OpenAPI::OAIDrive &drive);

        SpacesManager *_spaceManager;
        OpenAPI::OAIDrive _drive;

        SpaceImage *_image;

        bool _hasManyPersonalSpaces;

        friend class SpacesManager;
        friend class SpaceImage;
    };

} // OCC
} // GraphApi
