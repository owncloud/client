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
