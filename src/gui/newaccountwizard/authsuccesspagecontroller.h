/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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

private:
    void buildPage();

    QWizardPage *_page = nullptr;
};
}
