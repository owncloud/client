/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "folderwizard.h"

#include "libsync/account.h"

#include <QPointer>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QStringList>

namespace OCC {
Q_DECLARE_LOGGING_CATEGORY(lcFolderWizard);

class FolderWizardPrivate
{
public:
    FolderWizardPrivate(FolderWizard *q, Account *account);
    static QString formatWarnings(const QStringList &warnings, bool isError = false);

    QString initialLocalPath() const;

    // todo: #44 this used to be calculated by the folderWizardRemotePath impl, which was oc10 specific and is not longer used
    // However! the folder defition still wants this value so I'm leaving this dummy here for now (which is the value returned for non oc10
    // accounts), to be investigated later
    QString remotePath() const { return QString(); }

    uint32_t priority() const;

    QString defaultSyncRoot() const;

    QUuid uuid() const;
    QUrl davUrl() const;
    QString spaceId() const;
    bool useVirtualFiles() const;
    QString displayName() const;

private:
    Q_DECLARE_PUBLIC(FolderWizard)
    FolderWizard *q_ptr;

    QPointer<Account> _account;
    class SpacesPage *_spacesPage = nullptr;
    class FolderWizardLocalPath *_folderWizardSourcePage = nullptr;
    class FolderWizardSelectiveSync *_folderWizardSelectiveSyncPage = nullptr;
};


class FolderWizardPage : public QWizardPage
{
    Q_OBJECT
public:
    FolderWizardPage(FolderWizardPrivate *parent)
        : QWizardPage(nullptr)
        , _parent(parent)
    {
    }

protected:
    inline FolderWizardPrivate *folderWizardPrivate() const
    {
        return _parent;
    }

private:
    FolderWizardPrivate *_parent;
};
}
