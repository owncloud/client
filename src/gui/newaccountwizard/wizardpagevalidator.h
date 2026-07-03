/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace OCC {

/** this is a solution to allow sharing the page controllers' validation routine without exposing the page controller itself.
 *  the wizard page controllers will implement this interface and the wizard will call the validate function in an override of QWizard::validateCurrentPage
 */
class WizardPageValidator
{
public:
    virtual ~WizardPageValidator() { }

    /**
     * @brief validate the data of a wizard page
     * @return true if the validation succeeds, false if it does not
     */
    virtual bool validate() = 0;
};
}
