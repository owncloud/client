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
#include "authsuccesspagecontroller.h"

#include "theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWizardPage>

namespace OCC {


AuthSuccessPageController::AuthSuccessPageController(QWizardPage *page, QObject *parent)
    : QObject{parent}
    , _page(page)
{
    buildPage();
}

void AuthSuccessPageController::buildPage()
{
    if (!_page)
        return;

    QString appName = Theme::instance()->appNameGUI();

    QLabel *logoLabel = new QLabel({}, _page);
    logoLabel->setPixmap(Theme::instance()->wizardHeaderLogo().pixmap(200, 100));
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    logoLabel->setAccessibleName(tr("Application Name Logo %1").arg(appName));

    QLabel *instructionLabel = new QLabel(tr("You're all set! Open %1 and get started.").arg(appName), _page);
    QFont font = instructionLabel->font();
    font.setPixelSize(20);
    font.setWeight(QFont::DemiBold);
    instructionLabel->setFont(font);
    instructionLabel->setWordWrap(true);
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QPushButton *advancedSettingsButton = new QPushButton(tr("Advanced Settings"), _page);
    advancedSettingsButton->setFlat(true);
    // ugh I hate this but...
    advancedSettingsButton->setFocusPolicy(Qt::NoFocus);
    QFont buttonFont = advancedSettingsButton->font();
    buttonFont.setPixelSize(14);
    buttonFont.setUnderline(true);
    advancedSettingsButton->setFont(buttonFont);
    QPalette buttonPalette = advancedSettingsButton->palette();
    QColor highlightColor = buttonPalette.color(QPalette::Highlight);
    buttonPalette.setColor(QPalette::Normal, QPalette::ButtonText, highlightColor);
    // I hoped this would change the text on eg hover or focus. Nope. just changes it overall
    QColor accentColor = buttonPalette.color(QPalette::Accent);
    //  buttonPalette.setColor(QPalette::Active, QPalette::ButtonText, accentColor);
    buttonPalette.setColor(QPalette::BrightText, accentColor);
    advancedSettingsButton->setPalette(buttonPalette);
    advancedSettingsButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QLabel *footerLogoLabel = nullptr;
    if (!Theme::instance()->wizardFooterLogo().isNull()) {
        footerLogoLabel = new QLabel({}, _page);
        footerLogoLabel->setPixmap(Theme::instance()->wizardFooterLogo().pixmap(100, 52));
        footerLogoLabel->setAlignment(Qt::AlignCenter);
        footerLogoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        // this is not the same logo as the header logo but we have no idea what this logo could be.
        // this is the designer's suggestion so far
        footerLogoLabel->setAccessibleName(tr("Additional logo defined by the organization"));
    }

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(50, 0, 50, 0);
    layout->setSpacing(12);
    layout->addStretch(1);
    layout->addWidget(logoLabel, Qt::AlignCenter);
    layout->addSpacing(16);
    layout->addWidget(instructionLabel, Qt::AlignCenter);

    QHBoxLayout *advancedSettingsLayout = new QHBoxLayout();
    advancedSettingsLayout->addStretch(1);
    advancedSettingsLayout->addWidget(advancedSettingsButton);
    advancedSettingsLayout->addStretch(1);
    layout->addLayout(advancedSettingsLayout);

    if (footerLogoLabel)
        layout->addWidget(footerLogoLabel, Qt::AlignCenter);
    layout->addStretch(1);
    _page->setLayout(layout);
}

bool AuthSuccessPageController::validate()
{
    return true;
}
}
