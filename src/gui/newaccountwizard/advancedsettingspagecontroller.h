/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "newaccountenums.h"
#include "wizardpagevalidator.h"

#include <QObject>

class QWizardPage;
class QButtonGroup;
class QLineEdit;
class QLabel;
class QPushButton;

namespace OCC {

struct AdvancedSettingsResult
{
    NewAccount::SyncType syncType = NewAccount::SyncType::NONE;
    QString syncRoot;
};

class AdvancedSettingsPageController : public QObject, public WizardPageValidator
{
    Q_OBJECT

public:
    explicit AdvancedSettingsPageController(QWizardPage *page, QObject *parent);
    bool validate() override;
    AdvancedSettingsResult defaultResult();

Q_SIGNALS:
    void success(const OCC::AdvancedSettingsResult &result);

private:
    void gatherSyncInfo();
    void buildPage();
    void showFolderPicker();
    void onRootDirFieldEdited();
    void syncTypeChanged(int id);
    bool validateSyncRoot(const QString &rootPath);

    NewAccount::SyncType _defaultSyncType = NewAccount::SyncType::NONE;
    QString _defaultSyncRoot;
    bool _vfsIsAvailable = false;
    bool _forceVfs = false;
    bool _lastHandEditedRootFailed = false;

    AdvancedSettingsResult _results;

    QWizardPage *_page = nullptr;
    QButtonGroup *_buttonGroup = nullptr;
    QLineEdit *_rootDirEdit = nullptr;
    QPushButton *_folderButton;
    QLabel *_errorField;
};
}
Q_DECLARE_METATYPE(OCC::AdvancedSettingsResult)
// this type id is throwaway, we just use it to ensure we declare the meta type only ONCE
// also this is only required to use the type cross thread, or with QSignalSpy during testing
static const int advancedSettingsResultTypeId = qRegisterMetaType<OCC::AdvancedSettingsResult>();
