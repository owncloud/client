/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "gui/folder.h"
#include "gui/folderwizard/folderwizard_p.h"

class Ui_FolderWizardSourcePage;
namespace OCC {


/**
 * @brief Page to ask for the local source folder
 * @ingroup gui
 */
class FolderWizardLocalPath : public FolderWizardPage
{
    Q_OBJECT
public:
    explicit FolderWizardLocalPath(FolderWizardPrivate *parent);
    ~FolderWizardLocalPath() override;

    bool isComplete() const override;
    void initializePage() override;

    QString localPath() const;
protected Q_SLOTS:
    void slotChooseLocalFolder();

private:
    Ui_FolderWizardSourcePage *_ui;
    QMap<QString, Folder *> _folderMap;
};

}
