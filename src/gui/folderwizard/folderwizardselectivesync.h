/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "gui/folderwizard/folderwizard_p.h"

namespace OCC {

class Account;
class SelectiveSyncWidget;

/**
 * @brief The FolderWizardSelectiveSync class
 * @ingroup gui
 */
class FolderWizardSelectiveSync : public FolderWizardPage
{
    Q_OBJECT
public:
    explicit FolderWizardSelectiveSync(Account *account, FolderWizardPrivate *parent);
    ~FolderWizardSelectiveSync() override;

    bool validatePage() override;

    void initializePage() override;
    bool useVirtualFiles() const;

    const QSet<QString> &selectiveSyncBlackList() const;

private slots:
    void slotVfsStateChanged(Qt::CheckState state);

private:
    SelectiveSyncWidget *_selectiveSync;
    QCheckBox *_virtualFilesCheckBox = nullptr;
    QSet<QString> _selectiveSyncBlackList;
};

}
