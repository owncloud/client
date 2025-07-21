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
#pragma once

#include "wizardpagevalidator.h"
#include <QObject>

class QWizardPage;
class QButtonGroup;
class QLineEdit;
class QLabel;

namespace OCC {

// I am not making this a strongly typed enum class as it's only used to support the radio button ids in the gui.
// an old fashioned enum does not require casts back and forth, which for this use case is a good thing.
enum SyncType { NONE, USE_VFS, SYNC_ALL, SELECTIVE_SYNC };

struct AdvancedSettingsResult
{
    SyncType _syncType = SyncType::NONE;
    QString _syncRoot;
};

class AdvancedSettingsPageController : public QObject, public WizardPageValidator
{
    Q_OBJECT

public:
    explicit AdvancedSettingsPageController(QWizardPage *page, QObject *parent);
    bool validate() override;

Q_SIGNALS:
    void success(const AdvancedSettingsResult &result);

private:
    void gatherSyncInfo();
    void buildPage();
    void showFolderPicker();
    void onRootDirFieldEdited();
    bool validateSyncRoot(const QString &rootPath);

    SyncType _defaultSyncType = SyncType::NONE;
    QString _defaultSyncRoot;
    bool _vfsIsAvailable = false;
    bool _forceVfs = false;
    bool _lastHandEditedRootFailed = false;

    AdvancedSettingsResult _results;

    QWizardPage *_page = nullptr;
    QButtonGroup *_buttonGroup = nullptr;
    QLineEdit *_rootDirEdit = nullptr;
    QLabel *_errorField;
};
}
