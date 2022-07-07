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

#pragma once

#include "libsync/accountfwd.h"

#include <QWizard>


class QCheckBox;

namespace OCC {

class SelectiveSyncWidget;

/**
 * @brief The FolderWizardSelectiveSync class
 * @ingroup gui
 */
class FolderWizardSelectiveSync : public QWizardPage
{
    Q_OBJECT
public:
    explicit FolderWizardSelectiveSync(const AccountPtr &account, QWidget *parent = nullptr);
    ~FolderWizardSelectiveSync() override;

    bool validatePage() override;

    void initializePage() override;
    void cleanupPage() override;
    bool useVirtualFiles() const;

private slots:
    void virtualFilesCheckboxClicked();

private:
    SelectiveSyncWidget *_selectiveSync;
    QCheckBox *_virtualFilesCheckBox = nullptr;
};

}
