/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "owncloudlib.h"

#include <OAIDrive.h>

#include <QIcon>
#include <QList>
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
        QString description() const;
        QUrl webUrl() const;
        QString id() const;
        QUrl webDavUrl() const;
        QString eTag() const;
        OpenAPI::OAIQuota quota() const;

        QUuid accountId() const;

        /***
         * Assign a sortPriority to a drive, used for sorting
         */
        uint32_t sortPriority() const;

        /**
         * Whether a drive object has been deleted.
         */
        bool disabled() const;

        SpaceImage *image() const;


    Q_SIGNALS:
        void imageChanged();

    protected:
        QList<OpenAPI::OAIDriveItem> getSpecialItems() const;


    private:
        Space(SpacesManager *spaceManager, const OpenAPI::OAIDrive &drive, bool hasManyPersonalSpaces);
        bool setDrive(const OpenAPI::OAIDrive &drive);

        SpacesManager *_spaceManager;
        QUuid _accountId;
        OpenAPI::OAIDrive _drive;

        SpaceImage *_image;

        bool _hasManyPersonalSpaces;

        friend class SpacesManager;
        friend class SpaceImage;
    };

} // OCC
} // GraphApi
