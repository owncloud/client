/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "newversionavailablewidget.h"
#include "theme.h"
#include "ui_newversionavailablewidget.h"

#include <QDialogButtonBox>
#include <QPushButton>

namespace OCC {

NewVersionAvailableWidget::NewVersionAvailableWidget(QWidget *parent, const QString &statusMessage)
    : QDialog(parent)
    , _ui(new ::Ui::Ui_NewVersionAvailableWidget)
{
    _ui->setupUi(this);

    _ui->icon->setPixmap(Theme::instance()->applicationIcon().pixmap(128, 128));
    _ui->label->setText(statusMessage);

    QPushButton *skipButton = _ui->buttonBox->addButton(tr("Skip this version"), QDialogButtonBox::ResetRole);
    QPushButton *getUpdateButton = _ui->buttonBox->addButton(tr("Get update"), QDialogButtonBox::AcceptRole);
    QPushButton *rejectButton = _ui->buttonBox->addButton(tr("Skip this time"), QDialogButtonBox::RejectRole);

    connect(skipButton, &QAbstractButton::clicked, this, &NewVersionAvailableWidget::skipVersion);
    connect(rejectButton, &QAbstractButton::clicked, this, &QDialog::reject);
    connect(getUpdateButton, &QAbstractButton::clicked, this, &QDialog::accept);
}

NewVersionAvailableWidget::~NewVersionAvailableWidget()
{
    delete _ui;
}

void NewVersionAvailableWidget::skipVersion()
{
    Q_EMIT versionSkipped();
    Q_EMIT finished(QDialog::Rejected);
}

/*void NewVersionAvailableWidget::notNow()
{
    Q_EMIT noUpdateNow();
    Q_EMIT finished();
}

void NewVersionAvailableWidget::getUpdate()
{
    Q_EMIT updateNow();
    Q_EMIT finished();
}*/

} // OCC namespace
