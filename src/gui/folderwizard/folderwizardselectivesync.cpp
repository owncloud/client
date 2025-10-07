/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
 * Copyright (C) by Duncan Mac-Vicar P. <duncan@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "folderwizardselectivesync.h"

#include "folderwizard.h"
#include "folderwizard_p.h"

#include "gui/application.h"
#include "gui/selectivesyncwidget.h"

#include "libsync/theme.h"

#include "common/vfs.h"

#include <QCheckBox>
#include <QVBoxLayout>


using namespace OCC;

FolderWizardSelectiveSync::FolderWizardSelectiveSync(Account *account, FolderWizardPrivate *parent)
    : FolderWizardPage(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    _selectiveSync = new SelectiveSyncWidget(account, this);
    layout->addWidget(_selectiveSync);

    if (Theme::instance()->showVirtualFilesOption() && VfsPluginManager::instance().bestAvailableVfsMode() == Vfs::WindowsCfApi) {
        _virtualFilesCheckBox = new QCheckBox(tr("Use virtual files instead of downloading content immediately"));
        connect(_virtualFilesCheckBox, &QCheckBox::checkStateChanged, this, &FolderWizardSelectiveSync::slotVfsStateChanged);
        _virtualFilesCheckBox->setChecked(true);
        layout->addWidget(_virtualFilesCheckBox);
        _virtualFilesCheckBox->setDisabled(Theme::instance()->forceVirtualFilesOption());
    }
}

FolderWizardSelectiveSync::~FolderWizardSelectiveSync()
{
}


void FolderWizardSelectiveSync::initializePage()
{
    QString targetPath = static_cast<FolderWizard *>(wizard())->d_func()->remotePath();
    QString alias = QFileInfo(targetPath).fileName();
    if (alias.isEmpty())
        alias = Theme::instance()->appName();
    _selectiveSync->setDavUrl(dynamic_cast<FolderWizard *>(wizard())->d_func()->davUrl());
    _selectiveSync->setFolderInfo(targetPath, alias);
    QWizardPage::initializePage();
}

bool FolderWizardSelectiveSync::validatePage()
{
    if (!useVirtualFiles()) {
        _selectiveSyncBlackList = _selectiveSync->createBlackList();
    }
    return true;
}

bool FolderWizardSelectiveSync::useVirtualFiles() const
{
    return _virtualFilesCheckBox && _virtualFilesCheckBox->isChecked();
}

const QSet<QString> &FolderWizardSelectiveSync::selectiveSyncBlackList() const
{
    return _selectiveSyncBlackList;
}

void FolderWizardSelectiveSync::slotVfsStateChanged(Qt::CheckState state)
{
    // Enable the selective sync widget when VFS is *not* used, disable it if VFS *is* used.
    _selectiveSync->setEnabled(state == Qt::Unchecked);
}
