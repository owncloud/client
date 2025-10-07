/*
 * Copyright (C) by Klaas Freitag <freitag@kde.org>
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

#include "accountstate.h"
#include "progressdispatcher.h"

#include <QPointer>
#include <QAbstractItemModel>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QQuickImageProvider>
#include <QtQml/QQmlEngine>

class QNetworkReply;


namespace OCC {

Q_DECLARE_LOGGING_CATEGORY(lcFolderStatus)

class Folder;
class PropfindJob;
namespace {
    class SubFolderInfo;
}
/**
 * @brief The FolderStatusModel class
 * @ingroup gui
 */
class FolderStatusModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    enum class Roles {
        AccessibleDescriptionRole = Qt::AccessibleDescriptionRole,
        DisplayName = Qt::UserRole + 1, // must be 0 as it is also used from the default delegate
        Subtitle,
        FolderErrorMsg,
        SyncProgressOverallPercent,
        SyncProgressOverallString,
        SyncProgressItemString,
        Priority,
        Quota,
        FolderStatusIcon,
        Folder
    };
    Q_ENUMS(Roles)

    FolderStatusModel(AccountState *accountState, QObject *parent = nullptr);

    // todo: #45 this only "needs" to exist as if it is removed there are gcc errors on the unique_ptr for the SubFolderInfo (which is reported as "incomplete"
    // type?) in the anonymous namspace
    ~FolderStatusModel() override;

    Folder *folder(const QModelIndex &index) const;

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
    void slotUpdateFolderState(Folder *);
    void resetFolders();
    void slotSetProgress(const ProgressInfo &progress, Folder *f);

private Q_SLOTS:
    void slotFolderSyncStateChange(Folder *f);

private:
    int indexOf(Folder *f) const;

    QPointer<AccountState> _accountState;

    // todo: #45 I don't see why these need to be unique pointers instead of simple instances.
    std::vector<std::unique_ptr<SubFolderInfo>> _folders;
};


} // namespace OCC
