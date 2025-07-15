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

    // get custom buttons according to functionality. the controller needs to actively hide and show these depending on
    // which stage of the progression we are in, and more importantly, it needs to connect to their clicked signals to
    // deliver the special functionality associated with each
    // alternate: it may be simpler to rename the "next" button on the oauth page and just auto-trigger that on first visit.
    // the more I think about it the better I like that idea, but let's see.
    /** conveniently get the open browser button without having to know which custom button it's associated with */
    QAbstractButton *browserButton();
    /** conveniently get the advanced settings button without having to know which custom button it's associated with */
    QAbstractButton *advancedButton();

private:
    QHash<QWizardPage *, WizardPageValidator *> _pageValidators;
};
}
