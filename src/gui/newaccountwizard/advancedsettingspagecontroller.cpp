/*
 * Copyright (C) Lisa Reese <lisa.reese@kiteworks.com>
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
#include "advancedsettingspagecontroller.h"
#include "common/vfs.h"
#include "folderman.h"

#include "theme.h"

#include <QButtonGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QWizardPage>

namespace OCC {


AdvancedSettingsPageController::AdvancedSettingsPageController(QWizardPage *page, QObject *parent)
    : QObject{parent}
    , _page(page)
{
    gatherSyncInfo();
    buildPage();
}

void AdvancedSettingsPageController::buildPage()
{
    if (!_page)
        return;

    _buttonGroup = new QButtonGroup(_page);

    QLabel *titleLabel = new QLabel(tr("Advanced settings"), _page);
    QFont titleFont = titleLabel->font();
    titleFont.setPixelSize(20);
    titleFont.setWeight(QFont::DemiBold);
    titleLabel->setFont(titleFont);
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QLabel *syncOptionsLabel = new QLabel(tr("Sync and download options"), _page);
    QLabel *syncRootLabel = new QLabel(tr("Folder location"));
    QFont sectionFont = syncOptionsLabel->font();
    sectionFont.setPixelSize(16);
    sectionFont.setWeight(QFont::DemiBold);
    syncOptionsLabel->setFont(sectionFont);
    syncRootLabel->setFont(sectionFont);
    syncOptionsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    syncRootLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);


    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(50, 0, 50, 0);
    layout->setSpacing(12);
    layout->addStretch(1);
    layout->addWidget(titleLabel, Qt::AlignLeft);
    layout->addSpacing(8);
    layout->addWidget(syncOptionsLabel, Qt::AlignLeft);

    if (_vfsIsAvailable) {
        QRadioButton *vfsButton = new QRadioButton(tr("Only sync and dowload files as you use them to save hard drive space"), _page);
        vfsButton->setFocusPolicy(Qt::StrongFocus);
        _buttonGroup->addButton(vfsButton, SyncType::USE_VFS);
        layout->addWidget(vfsButton, Qt::AlignLeft);
    }
    if (!_forceVfs) {
        QRadioButton *selectiveSyncButton = new QRadioButton(tr("Sync and download specific folders"), _page);
        selectiveSyncButton->setFocusPolicy(Qt::StrongFocus);
        QRadioButton *syncAllButton = new QRadioButton(tr("Automatically sync and download all current folders and files"), _page);
        syncAllButton->setFocusPolicy(Qt::StrongFocus);
        _buttonGroup->addButton(selectiveSyncButton, SyncType::SELECTIVE_SYNC);
        _buttonGroup->addButton(syncAllButton, SyncType::SYNC_ALL);
        layout->addWidget(selectiveSyncButton, Qt::AlignLeft);
        layout->addWidget(syncAllButton, Qt::AlignLeft);
    }

    Q_ASSERT(_buttonGroup->button(_syncType));
    _buttonGroup->button(_syncType)->setChecked(true);

    _rootDirEdit = new QLineEdit(_page);
    _rootDirEdit->setText(_syncRoot);
    _rootDirEdit->setFocusPolicy(Qt::StrongFocus);

    QPushButton *folderButton = new QPushButton(tr("Choose..."), _page);
    folderButton->setFocusPolicy(Qt::StrongFocus);
    connect(folderButton, &QPushButton::clicked, this, &AdvancedSettingsPageController::showFolderPicker);

    layout->addSpacing(8);
    layout->addWidget(syncRootLabel, Qt::AlignLeft);

    QHBoxLayout *folderPickerLayout = new QHBoxLayout();
    folderPickerLayout->addWidget(_rootDirEdit, Qt::AlignLeft);
    folderPickerLayout->addWidget(folderButton);
    layout->addLayout(folderPickerLayout);

    layout->addStretch(1);
    _page->setLayout(layout);
}

void AdvancedSettingsPageController::gatherSyncInfo()
{
    _vfsIsAvailable = VfsPluginManager::instance().bestAvailableVfsMode() == Vfs::WindowsCfApi;
    _forceVfs = _vfsIsAvailable && Theme::instance()->forceVirtualFilesOption();
    if (!_vfsIsAvailable)
        _syncType = SyncType::SYNC_ALL;
    else
        _syncType = SyncType::USE_VFS;

    _syncRoot = FolderMan::suggestSyncFolder(FolderMan::NewFolderType::SpacesSyncRoot, {});
}

bool AdvancedSettingsPageController::validate()
{
    AdvancedSettingsResult result;
    result._syncRoot = _rootDirEdit->text();
    result._syncType = static_cast<SyncType>(_buttonGroup->checkedId());
    return true;
}

void AdvancedSettingsPageController::showFolderPicker()
{
    //  _errorField->setText({});

    QString chosenRoot = QFileDialog::getExistingDirectory(_page->parentWidget(), tr("Select sync root"), _syncRoot);
    // do some checks on it to be sure it's a supported dir
    if (chosenRoot.isEmpty())
        return;

    QString errorMessageTemplate = tr("Invalid local download directory %1: %2.");

    if (!QDir::isAbsolutePath(chosenRoot)) {
        //   _errorField->setText(errorMessageTemplate.arg(chosenRoot, tr("path must be absolute")));
        return;
    }

    QString invalidPathErrorMessage = FolderMan::checkPathValidityRecursive(chosenRoot, FolderMan::NewFolderType::SpacesSyncRoot, {});
    if (!invalidPathErrorMessage.isEmpty()) {
        //  _errorField->setText(errorMessageTemplate.arg(chosenRoot, invalidPathErrorMessage));
        return;
    }

    _rootDirEdit->setText(chosenRoot);
}
}
