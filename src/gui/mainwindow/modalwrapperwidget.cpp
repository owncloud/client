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

#include "modalwrapperwidget.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtWidgets/qlabel.h>

namespace OCC {

ModalWrapperWidget::ModalWrapperWidget(QWidget *content, QWidget *parent)
    : QWidget{parent}
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    content->setParent(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(content);
    if (QDialog *asDialog = qobject_cast<QDialog *>(content)) {
        connect(asDialog, &QDialog::finished, this, &ModalWrapperWidget::finished);
    } else {
        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &ModalWrapperWidget::finished);
        // yes I know there is a default "close" button, but it emits rejected which can have consequences
        // I'm choosing close for the naming here because it doesn't imply any sort of "save" action, which will be important
        // wrt the settings view, which updates values "live" as they are changed, not committed on "ok" or "save"
        QPushButton *okButton = buttons->button(QDialogButtonBox::Ok);
        okButton->setObjectName("closeButton");
        okButton->setText(tr("Close"));
        layout->addWidget(buttons);
    }

    setLayout(layout);
}

}
