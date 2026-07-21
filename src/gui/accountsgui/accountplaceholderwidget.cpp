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

#include "accountplaceholderwidget.h"

#include <QLabel>
#include <QVBoxLayout>

#include "theme.h"

namespace OCC {

AccountPlaceholderWidget::AccountPlaceholderWidget(QWidget *parent)
    : QWidget{parent}
{
    setObjectName("accountPlaceholderWidget");
    QString appName = Theme::instance()->appNameGUI();

    QLabel *logoLabel = new QLabel({}, this);
    logoLabel->setPixmap(Theme::instance()->wizardHeaderLogo().pixmap(200, 100));
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    logoLabel->setAccessibleName(tr("%1 logo").arg(appName));

    QLabel *instructionLabel = new QLabel(
        tr("You don't have any connected accounts yet.\n\nPlease create an account by selecting the \"Add account...\" option on the More button menu."), this);
    instructionLabel->setWordWrap(true);
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QLabel *footerLogoLabel = nullptr;
    if (!Theme::instance()->wizardFooterLogo().isNull()) {
        footerLogoLabel = new QLabel({}, this);
        footerLogoLabel->setPixmap(Theme::instance()->wizardFooterLogo().pixmap(100, 52));
        footerLogoLabel->setAlignment(Qt::AlignCenter);
        footerLogoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        footerLogoLabel->setAccessibleName(tr("Additional logo defined by the organization"));
    }

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(50, 0, 50, 0);
    layout->setSpacing(12);
    layout->addStretch(1);
    layout->addWidget(logoLabel, Qt::AlignCenter);
    layout->addSpacing(16);
    layout->addWidget(instructionLabel, Qt::AlignCenter);
    if (footerLogoLabel)
        layout->addWidget(footerLogoLabel, Qt::AlignCenter);
    layout->addStretch(1);
    setLayout(layout);
}

}
