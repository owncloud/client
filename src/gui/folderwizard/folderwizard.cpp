/*
 * Copyright (C) by Duncan Mac-Vicar P. <duncan@kde.org>
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

#include "folderwizard.h"
#include "folderwizard_p.h"

#include "folderwizardlocalpath.h"
#include "folderwizardselectivesync.h"

#include "spacespage.h"

#include "account.h"
#include "common/asserts.h"
#include "common/vfs.h"
#include "gui/application.h"
#include "gui/settingsdialog.h"
#include "theme.h"

#include "gui/folderman.h"

#include "libsync/graphapi/space.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QUrl>


namespace OCC {

Q_LOGGING_CATEGORY(lcFolderWizard, "gui.folderwizard", QtInfoMsg)

QString FolderWizardPrivate::formatWarnings(const QStringList &warnings, bool isError)
{
    QString ret;
    if (warnings.count() == 1) {
        ret = isError ? QCoreApplication::translate("FolderWizard", "<b>Error:</b> %1").arg(warnings.first()) : QCoreApplication::translate("FolderWizard", "<b>Warning:</b> %1").arg(warnings.first());
    } else if (warnings.count() > 1) {
        QStringList w2;
        for (const auto &warning : warnings) {
            w2.append(QStringLiteral("<li>%1</li>").arg(warning));
        }
        ret = isError ? QCoreApplication::translate("FolderWizard", "<b>Error:</b><ul>%1</ul>").arg(w2.join(QString()))
                      : QCoreApplication::translate("FolderWizard", "<b>Warning:</b><ul>%1</ul>").arg(w2.join(QString()));
    }

    return ret;
}

// todo: #43
QString FolderWizardPrivate::defaultSyncRoot() const
{
    // this should never happen when we have set up the account using spaces - there is ALWAYS a default root when spaces are in play
    // and they are always in play so this check is bogus. todo: #43
    if (!_account->hasDefaultSyncRoot()) {
        const auto folderType = FolderMan::NewFolderType::SpacesSyncRoot; // todo: #43 : FolderMan::NewFolderType::OC10SyncRoot;
        return FolderMan::suggestSyncFolder(folderType, _account->uuid());
    } else {
        return _account->defaultSyncRoot();
    }
}

QUuid FolderWizardPrivate::uuid() const
{
    if (_account)
        return _account->uuid();
    return {};
}

FolderWizardPrivate::FolderWizardPrivate(FolderWizard *q, Account *account)
    : q_ptr(q)
    , _account(account)
    , _folderWizardSourcePage(new FolderWizardLocalPath(this))
    , _folderWizardSelectiveSyncPage(nullptr)
{
    if (_account) {
        _spacesPage = new SpacesPage(account->spacesManager(), q);
        q->setPage(FolderWizard::Page_Space, _spacesPage);
    }

    q->setPage(FolderWizard::Page_Source, _folderWizardSourcePage);

    // When VFS is available (currently only with Windows' CFApi), and it is forced on, Spaces are meant to be synced as a whole.
    const bool showPage = VfsPluginManager::instance().bestAvailableVfsMode() != Vfs::WindowsCfApi || !Theme::instance()->forceVirtualFilesOption();
    if (showPage) {
        _folderWizardSelectiveSyncPage = new FolderWizardSelectiveSync(_account, this);
        q->setPage(FolderWizard::Page_SelectiveSync, _folderWizardSelectiveSyncPage);
    }
}

QString FolderWizardPrivate::initialLocalPath() const
{
    return FolderMan::findGoodPathForNewSyncFolder(
        defaultSyncRoot(), _spacesPage->currentSpace()->displayName(), FolderMan::NewFolderType::SpacesSyncRoot, _account->uuid());
}

uint32_t FolderWizardPrivate::priority() const
{
    return _spacesPage->currentSpace()->priority();
}

QUrl FolderWizardPrivate::davUrl() const
{
    auto url = _spacesPage->currentSpace()->webdavUrl();
    if (!url.path().endsWith(QLatin1Char('/'))) {
        url.setPath(url.path() + QLatin1Char('/'));
    }
    return url;
}

QString FolderWizardPrivate::spaceId() const
{
    return _spacesPage->currentSpace()->id();
}

QString FolderWizardPrivate::displayName() const
{
    return _spacesPage->currentSpace()->displayName();
}

bool FolderWizardPrivate::useVirtualFiles() const
{
    const auto mode = VfsPluginManager::instance().bestAvailableVfsMode();
    const bool useVirtualFiles = (Theme::instance()->forceVirtualFilesOption() && mode == Vfs::WindowsCfApi) || (_folderWizardSelectiveSyncPage && _folderWizardSelectiveSyncPage->useVirtualFiles());
    if (useVirtualFiles) {
        const auto availability = Vfs::checkAvailability(initialLocalPath(), mode);
        if (!availability) {
            auto msg = new QMessageBox(QMessageBox::Warning, FolderWizard::tr("Virtual files are not available for the selected folder"), availability.error(), QMessageBox::Ok, ocApp()->gui()->settingsDialog());
            msg->setAttribute(Qt::WA_DeleteOnClose);
            msg->open();
            return false;
        }
    }
    return useVirtualFiles;
}

FolderWizard::FolderWizard(Account *account, QWidget *parent)
    : QWizard(parent)
    , d_ptr(new FolderWizardPrivate(this, account))
{
    setWindowTitle(tr("Add Folder Sync Connection"));
    setOptions(QWizard::CancelButtonOnLeft);
    setButtonText(QWizard::FinishButton, tr("Add Sync Connection"));
    setWizardStyle(QWizard::ModernStyle);
}

FolderMan::SyncConnectionDescription FolderWizard::result()
{
    Q_D(FolderWizard);

    const QString localPath = d->_folderWizardSourcePage->localPath();
    if (!d->_account->hasDefaultSyncRoot()) {
        if (FileSystem::isChildPathOf(localPath, d->defaultSyncRoot())) {
            d->_account->setDefaultSyncRoot(d->defaultSyncRoot());
            if (!QFileInfo::exists(d->defaultSyncRoot())) {
                OC_ASSERT(QDir().mkpath(d->defaultSyncRoot()));
            }
        }
    }

    // clang-format off
    return {
        d->davUrl(),
        d->spaceId(),
        localPath,
        d->remotePath(),
        d->displayName(),
        d->useVirtualFiles(),
        d->priority(),
        d->_folderWizardSelectiveSyncPage ? d->_folderWizardSelectiveSyncPage->selectiveSyncBlackList() : QSet<QString>{}
    };
    // clang-format on
}

} // end namespace
