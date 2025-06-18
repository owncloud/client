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
#include "newaccountwizard.h"

#include "wizardpagevalidator.h"
#include <qabstractbutton.h>

namespace OCC {

NewAccountWizard::NewAccountWizard(QWidget *parent)
    : QWizard(parent)
{
}

QAbstractButton *NewAccountWizard::browserButton()
{
    return button(WizardButton::CustomButton1);
}

QAbstractButton *NewAccountWizard::advancedButton()
{
    return button(WizardButton::CustomButton2);
}

bool NewAccountWizard::validateCurrentPage()
{
    QWizardPage *page = currentPage();
    if (page && _pageValidators.contains(page)) {
        return _pageValidators[page]->validate();
    }
    return false;
}

int NewAccountWizard::addPage(QWizardPage *page, WizardPageValidator *validator)
{
    _pageValidators.insert(page, validator);
    return QWizard::addPage(page);
}
}
