/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
