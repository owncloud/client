/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QWizard>

namespace OCC {
class WizardPageValidator;

class NewAccountWizard : public QWizard
{
    Q_OBJECT

public:
    NewAccountWizard(QWidget *parent);

    bool validateCurrentPage() override;

    /** overload of QWizard::addPage which also accepts the associated page validator */
    int addPage(QWizardPage *page, WizardPageValidator *validator);

public Q_SLOTS:

    void reject() override;

private:
    QHash<QWizardPage *, WizardPageValidator *> _pageValidators;
};
}
