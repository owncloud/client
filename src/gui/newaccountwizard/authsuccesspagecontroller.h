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

#include <QObject>

#include "wizardpagevalidator.h"

class QWizardPage;

namespace OCC {
class AuthSuccessPageController : public QObject, public WizardPageValidator
{
    Q_OBJECT
public:
    explicit AuthSuccessPageController(QWizardPage *page, QObject *parent);
    bool validate() override;

Q_SIGNALS:
    void requestAdvancedSettings();

private:
    void buildPage();

    QWizardPage *_page = nullptr;
};
}
