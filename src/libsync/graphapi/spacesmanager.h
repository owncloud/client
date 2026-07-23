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

        bool isReady() const { return _ready; }
        int spacesCount() const { return _spaces.count(); }

    Q_SIGNALS:
        void spaceChanged(Space *space) const;
        // I think this will go
        void updated(Account *account);
        void ready() const;
        void spaceAdded(QUuid accountId, OCC::GraphApi::Space *space);
        void spaceAboutToBeRemoved(QUuid accountId, OCC::GraphApi::Space *space);
        // these are emitted after any/all processing of active spaces is complete, so eg for the space deleted
        // we can only provide space id's since the pointers are gone.
        void spacesAdded(QUuid accountId, QList<OCC::GraphApi::Space *> spaces, int totalSpaceCount);
        void spacesRemoved(QUuid accountId, QList<QString> deletedSpaces, int totalSpaceCount);

    private:
        void refresh();

        QPointer<Account> _account;
        QTimer *_refreshTimer;
        QHash<QString, Space *> _spaces;
        bool _ready = false;
    };

}
}
