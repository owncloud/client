/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "updatedownloadedwidget.h"
#include "theme.h"
#include "ui_updatedownloadedwidget.h"

#include <QDialogButtonBox>
#include <QPushButton>

namespace OCC {

UpdateDownloadedWidget::UpdateDownloadedWidget(QWidget *parent, const QString &statusMessage)
    : QDialog(parent)
    , _ui(new ::Ui::UpdateDownloadedWidget)
{
    _ui->setupUi(this);

    _ui->iconLabel->setPixmap(Theme::instance()->applicationIcon().pixmap(128, 128));
    _ui->iconLabel->setText(QString());

    _ui->descriptionLabel->setText(statusMessage);

    const auto noButton = _ui->buttonBox->button(QDialogButtonBox::No);
    const auto yesButton = _ui->buttonBox->button(QDialogButtonBox::Yes);

    noButton->setText(tr("Restart later"));

    yesButton->setText(tr("Restart now"));
    yesButton->setDefault(true);
}

UpdateDownloadedWidget::~UpdateDownloadedWidget()
{
    delete _ui;
}

/*
void UpdateDownloadedWidget::accept()
{
    Q_EMIT accepted();
    Q_EMIT finished();
}

void UpdateDownloadedWidget::reject()
{
    Q_EMIT finished();
}
*/
} // OCC namespace
