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

#include "libsync/graphapi/space.h"

#include <OAIDrive.h>

class QTimer;

namespace OCC {

class Account;

namespace GraphApi {

    class OWNCLOUDSYNC_EXPORT SpacesManager : public QObject
    {
        Q_OBJECT

    public:
        SpacesManager(Account *parent);

        Space *space(const QString &id) const;

        QVector<Space *> spaces() const;

        // todo DC-150: remove this accessor and take responsibility for running job to retrieve/update space image as needed
        // once that is complete we can get rid of the account memeber entirely (and even revert the parent arg to a simple QObject)
        // by passing the value for hasManyPersonalSpaces to this via ctr
        Account *account() const;

        /**
         * Only relevant during bootstraping or when disconnected
         */
        void checkReady();

    Q_SIGNALS:
        void spaceChanged(Space *space) const;
        // I think this will go
        void updated(Account *account);
        void ready() const;
        void spaceAdded(OCC::GraphApi::Space *space);
        void spaceAboutToBeRemoved(OCC::GraphApi::Space *space);
        // these are emitted after any/all processing of active spaces is complete, so eg for the space deleted
        // we can only provide space id's since the pointers are gone.
        void spacesAdded(QList<OCC::GraphApi::Space *> spaces);
        void spacesRemoved(QList<QString> deletedSpaces);

    private:
        void refresh();

        QPointer<Account> _account;
        QTimer *_refreshTimer;
        QMap<QString, Space *> _spacesMap;
        bool _ready = false;
    };

}
}
