/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
