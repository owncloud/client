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

#include <QAbstractButton>
#include <QMessageBox>

namespace OCC {

NewAccountWizard::NewAccountWizard(QWidget *parent)
    : QWizard(parent)
{
}

bool NewAccountWizard::validateCurrentPage()
{
    QWizardPage *page = currentPage();
    if (page && _pageValidators.contains(page)) {
        // fix the problem that user can click next button multiple times
        // it should auto re-enable on forward, back, but won't let a successful validation be repeated on same page
        button(WizardButton::NextButton)->setEnabled(false);
        bool success = _pageValidators[page]->validate();
        if (!success)
            button(WizardButton::NextButton)->setEnabled(true);
        return success;
    }
    return false;
}

int NewAccountWizard::addPage(QWizardPage *page, WizardPageValidator *validator)
{
    _pageValidators.insert(page, validator);
    return QWizard::addPage(page);
}

void NewAccountWizard::reject()
{
    QMessageBox::StandardButton result = QMessageBox::question(
        this, tr("Cancel Setup"), tr("Do you really want to cancel the account setup?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (result == QMessageBox::Yes)
        QWizard::reject();
}
}
