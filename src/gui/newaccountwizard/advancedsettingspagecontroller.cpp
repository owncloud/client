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
    QLabel *syncRootLabel = new QLabel(tr("Download location"));
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

    QRadioButton *vfsButton = new QRadioButton(tr("Sync and download files as you use them (saves hard drive space)"), _page);
    vfsButton->setFocusPolicy(Qt::StrongFocus);
    _buttonGroup->addButton(vfsButton, NewAccount::SyncType::USE_VFS);
    layout->addWidget(vfsButton, Qt::AlignLeft);

    QRadioButton *selectiveSyncButton = new QRadioButton(tr("Sync and download folders manually"), _page);
    selectiveSyncButton->setFocusPolicy(Qt::StrongFocus);
    _buttonGroup->addButton(selectiveSyncButton, NewAccount::SyncType::SELECTIVE_SYNC);
    layout->addWidget(selectiveSyncButton, Qt::AlignLeft);

    QRadioButton *syncAllButton = new QRadioButton(tr("Automatically sync and download folders and files"), _page);
    syncAllButton->setFocusPolicy(Qt::StrongFocus);
    _buttonGroup->addButton(syncAllButton, NewAccount::SyncType::SYNC_ALL);
    layout->addWidget(syncAllButton, Qt::AlignLeft);

    connect(_buttonGroup, &QButtonGroup::idClicked, this, &AdvancedSettingsPageController::syncTypeChanged);

    // if the branding does not specify a background color for the wizard we should just let the normal system colors
    // rule. If not:
    QPalette radioPalette = vfsButton->palette();
    QColor textColor = _page->palette().color(QPalette::Text);
    if (Theme::instance()->wizardHeaderBackgroundColor().isValid()) {
        QColor disabledColor(textColor);
        // eh - I don't know if this will really work in all possible cases but I think it is the simplest way for now
        disabledColor.setAlpha(128);
        // unexpected, but these are the only ones that work to change the text color and the circle colors of the radio button
        // on windows - need to test other platforms for sure
        radioPalette.setColor(QPalette::WindowText, textColor);
        radioPalette.setColor(QPalette::Window, textColor);
        radioPalette.setColor(QPalette::Disabled, QPalette::Window, disabledColor);
    }
    // for this use case we want the text to remain "normal" regardless of branding. the only time we disable a button is when there
    // is only one choice
    radioPalette.setColor(QPalette::Disabled, QPalette::WindowText, textColor);
    vfsButton->setPalette(radioPalette);
    syncAllButton->setPalette(radioPalette);
    selectiveSyncButton->setPalette(radioPalette);

    if (!_vfsIsAvailable) {
        vfsButton->setVisible(false);
    } else if (_forceVfs) {
        vfsButton->setEnabled(false);
        selectiveSyncButton->setVisible(false);
        syncAllButton->setVisible(false);
    }

    Q_ASSERT(_buttonGroup->button(_defaultSyncType));
    _buttonGroup->button(_defaultSyncType)->setChecked(true);

    _rootDirEdit = new QLineEdit(_page);
    _rootDirEdit->setText(_defaultSyncRoot);
    QPalette dirPalette = _rootDirEdit->palette();
    dirPalette.setColor(QPalette::Base, dirPalette.color(QPalette::Button));
    dirPalette.setColor(QPalette::Text, dirPalette.color(QPalette::ButtonText));
    _rootDirEdit->setPalette(dirPalette);
    _rootDirEdit->setFocusPolicy(Qt::StrongFocus);
    _rootDirEdit->setAccessibleName(tr("Download location on the local machine"));
    // just clear the error if the user starts typing in the text edit
    connect(_rootDirEdit, &QLineEdit::textEdited, this, [this] {
        if (!_errorField->text().isEmpty())
            _errorField->setText({});
    });
    connect(_rootDirEdit, &QLineEdit::editingFinished, this, &AdvancedSettingsPageController::onRootDirFieldEdited);

    _folderButton = new QPushButton(tr("Browse..."), _page);
    _folderButton->setFocusPolicy(Qt::StrongFocus);
    _folderButton->setAccessibleDescription(tr("Browse for a download location"));
    connect(_folderButton, &QPushButton::clicked, this, &AdvancedSettingsPageController::showFolderPicker);

    layout->addSpacing(8);
    layout->addWidget(syncRootLabel, Qt::AlignLeft);

    QHBoxLayout *folderPickerLayout = new QHBoxLayout();
    folderPickerLayout->addWidget(_rootDirEdit, Qt::AlignLeft);
    folderPickerLayout->addWidget(_folderButton);
    layout->addLayout(folderPickerLayout);

    _errorField = new QLabel(QString(), _page);
    QPalette errorPalette = _errorField->palette();
    errorPalette.setColor(QPalette::Text, Qt::red);
    _errorField->setPalette(errorPalette);
    _errorField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _errorField->setWordWrap(true);
    _errorField->setAlignment(Qt::AlignLeft);
    _errorField->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    layout->addWidget(_errorField);

    layout->addStretch(1);
    _page->setLayout(layout);
}

void AdvancedSettingsPageController::gatherSyncInfo()
{
    _vfsIsAvailable = VfsPluginManager::instance().bestAvailableVfsMode() == Vfs::WindowsCfApi;
    _forceVfs = _vfsIsAvailable && Theme::instance()->forceVirtualFilesOption();
    if (!_vfsIsAvailable)
        _defaultSyncType = NewAccount::SyncType::SYNC_ALL;
    else
        _defaultSyncType = NewAccount::SyncType::USE_VFS;

    _defaultSyncRoot = FolderMan::suggestSyncFolder(FolderMan::NewFolderType::SpacesSyncRoot, {});
}

AdvancedSettingsResult AdvancedSettingsPageController::defaultResult()
{
    AdvancedSettingsResult result;
    result.syncRoot = _defaultSyncRoot;
    result.syncType = _defaultSyncType;
    return result;
}

bool AdvancedSettingsPageController::validate()
{
    // this is a safety net because for unknown reasons, on windows when you hit enter to commit hand edited text
    // in the QLineEdit, it ALSO triggers the finish button. No idea but this should block the finish so the user
    // can see their last choice failed, and that the root has been reset to the default.
    if (_lastHandEditedRootFailed)
        return false;

    // normally I don't like taking values directly from the gui but in this case, it would be complete overkill to
    // create a dedicated model just for these two values that only need to be collected when the user is done.
    AdvancedSettingsResult result;
    result.syncType = static_cast<NewAccount::SyncType>(_buttonGroup->checkedId());
    if (result.syncType == NewAccount::SyncType::SELECTIVE_SYNC)
        result.syncRoot.clear();
    else
        result.syncRoot = _rootDirEdit->text();
    Q_EMIT success(result);
    return true;
}

bool AdvancedSettingsPageController::validateSyncRoot(const QString &rootPath)
{
    QString errorMessageTemplate = tr("Invalid local download directory %1: %2");

    if (rootPath == QDir::homePath()) {
        _errorField->setText(errorMessageTemplate.arg(rootPath, tr("your user directory may not be chosen as the sync root.")));
        return false;
    }

    if (Utility::isMac()) {
        QString filesystemType = FileSystem::fileSystemForPath(rootPath);
        if (filesystemType != QStringLiteral("apfs")) {
            _errorField->setText(errorMessageTemplate.arg(rootPath, tr("path is not located on a supported Apple File System.")));
        }
    }

    if (!QDir::isAbsolutePath(rootPath)) {
        _errorField->setText(errorMessageTemplate.arg(rootPath, tr("path must be absolute.")));
        return false;
    }

    QString invalidPathErrorMessage = FolderMan::checkPathValidityRecursive(rootPath, FolderMan::NewFolderType::SpacesSyncRoot, {});
    if (!invalidPathErrorMessage.isEmpty()) {
        _errorField->setText(errorMessageTemplate.arg(rootPath, invalidPathErrorMessage));
        return false;
    }

    // I'm not testing the case that vfs is actually the chosen mode here as if vfs is available we should just block dirs that don't support it,
    // the idea being that eg if they change the mode in the page after they select the dir, or use selective sync with vfs later, at least we know the default
    // root they select here will not cause any issues down the road
    if (_vfsIsAvailable && !FolderMan::instance()->checkVfsAvailability(rootPath)) {
        _errorField->setText(errorMessageTemplate.arg(rootPath, tr("selected path does not support using virtual file system.")));
        return false;
    }

    return true;
}

void AdvancedSettingsPageController::showFolderPicker()
{
    _errorField->setText({});
    _lastHandEditedRootFailed = false;

    QDir defaultDir(_defaultSyncRoot);
    while (!defaultDir.exists())
        defaultDir.cdUp();
    QString chosenRoot = QFileDialog::getExistingDirectory(_page->parentWidget(), tr("Select sync root"), defaultDir.path());

    if (chosenRoot.isEmpty())
        return;

    if (validateSyncRoot(chosenRoot))
        _rootDirEdit->setText(chosenRoot);
}

void AdvancedSettingsPageController::onRootDirFieldEdited()
{
    QString chosenRoot = _rootDirEdit->text();
    if (!validateSyncRoot(chosenRoot)) {
        _rootDirEdit->setText(_defaultSyncRoot);
        _lastHandEditedRootFailed = true;
    } else {
        _lastHandEditedRootFailed = false;
        _errorField->setText({});
    }
}

void AdvancedSettingsPageController::syncTypeChanged(int id)
{
    bool enableFolderEditors = (id != NewAccount::SyncType::SELECTIVE_SYNC);

    _rootDirEdit->setEnabled(enableFolderEditors);
    _folderButton->setEnabled(enableFolderEditors);
}
}
