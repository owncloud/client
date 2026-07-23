/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QUrl>
#include <QWizard>

#include "gui/folderman.h"

class QCheckBox;
class QTreeWidgetItem;

class Ui_FolderWizardTargetPage;

namespace OCC {

class FolderWizardPrivate;

/**
 * @brief The FolderWizard class
 * @ingroup gui
 */
class FolderWizard : public QWizard
{
    Q_OBJECT
public:
    enum PageType {
        Page_Space,
        Page_Source,
        Page_Target,
        Page_SelectiveSync
    };
    Q_ENUM(PageType)

    explicit FolderWizard(Account *account, QWidget *parent);

    FolderMan::SyncConnectionDescription result();
    Q_DECLARE_PRIVATE(FolderWizard)

signals:
    void folderWizardAccepted(OCC::FolderMan::SyncConnectionDescription result);

private:
    void sendResult();


private:
    QScopedPointer<FolderWizardPrivate> d_ptr;
};

} // namespace OCC
