/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
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

#include "accountmodalwidget.h"
#include "ui_accountmodalwidget.h"

#include "gui/qmlutils.h"

#include <QDialog>

namespace OCC {

AccountModalWidget::AccountModalWidget(const QString &title, QWidget *widget, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AccountModalWidget)
{
    ui->setupUi(this);
    ui->groupBox->setTitle(title);
    ui->groupBox->layout()->addWidget(widget);

    // the internal widget may be a dialog, in which case we want to block adding "standard buttons" and
    // connect to the dialog accept/reject signals instead
    QDialog *dialog = qobject_cast<QDialog *>(widget);
    if (dialog) {
        _widgetHasButtons = true;
        connect(dialog, &QDialog::accepted, this, &AccountModalWidget::accept);
        connect(dialog, &QDialog::rejected, this, &AccountModalWidget::reject);
    } else {
        _widgetHasButtons = false;
        connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AccountModalWidget::accept);
        connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AccountModalWidget::reject);
    }
}


AccountModalWidget::AccountModalWidget(const QString &title, const QUrl &qmlSource, QObject *qmlContext, QWidget *parent)
    : AccountModalWidget(
          title,
          [&] {
              auto *out = new QmlUtils::OCQuickWidget;
              out->setOCContext(qmlSource, parent, qmlContext, QJSEngine::JavaScriptOwnership);
              return out;
          }(),
          parent)
{
}
void AccountModalWidget::setStandardButtons(QDialogButtonBox::StandardButtons buttons)
{
    if (!_widgetHasButtons)
        ui->buttonBox->setStandardButtons(buttons);
}

QPushButton *AccountModalWidget::addButton(const QString &text, QDialogButtonBox::ButtonRole role)
{
    if (!_widgetHasButtons)
        return ui->buttonBox->addButton(text, role);
    return nullptr;
}

void AccountModalWidget::accept()
{
    Q_EMIT accepted();
    Q_EMIT finished(this);
}

void AccountModalWidget::reject()
{
    Q_EMIT rejected();
    Q_EMIT finished(this);
}

} // OCC
