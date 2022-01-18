/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
 * Copyright (C) by Krzesimir Nowak <krzesimir@endocode.com>
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

#ifndef MIRALL_OWNCLOUD_SETUP_PAGE_H
#define MIRALL_OWNCLOUD_SETUP_PAGE_H

#include <QScopedPointer>
#include <QWizard>

#include "wizard/abstractcredswizardpage.h"
#include "wizard/owncloudwizard.h"
#include "wizard/owncloudwizardcommon.h"

class QLabel;
class QVariant;
class QProgressIndicator;
class QButtonGroup;
class Ui_OwncloudSetupPage;

namespace OCC {

/**
 * @brief The OwncloudSetupPage class
 * @ingroup gui
 */
class OwncloudSetupPage : public AbstractWizardPage
{
    Q_OBJECT
public:
    OwncloudSetupPage(OwncloudWizard *parent);
    ~OwncloudSetupPage() override;

    bool isComplete() const override;
    void initializePage() override;
    void setServerUrl(const QString &);
    void setAllowPasswordStorage(bool);
    bool validatePage() override;
    QString url() const;
    QString localFolder() const;
    void setRemoteFolder(const QString &remoteFolder);
    void setMultipleFoldersExist(bool exist);
    void setAuthType();

public slots:
    void setErrorString(const QString &);

protected slots:
    void slotUrlChanged(const QString &);
    void slotUrlEditFinished();

private:
    QScopedPointer<Ui_OwncloudSetupPage> _ui;

    QString _oCUrl;
    QButtonGroup *_selectiveSyncButtons;
};

} // namespace OCC

#endif
