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

namespace OCC {

/** this is a solution to allow sharing the page controllers' validation routine without exposing the page controller itself.
 *  the wizard page controllers will implement this interface and the wizard will call the validate function in an override of QWizard::validateCurrentPage
 */
class WizardPageValidator
{
public:
    /**
     * @brief validate the data of a wizard page
     * @return true if the validation succeeds, false if it does not
     */
    virtual bool validate() = 0;
};
}
