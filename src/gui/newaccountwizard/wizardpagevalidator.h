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

/** this is a possibly funky solution to allow sharing the page controllers' validation routines without actually sharing the page controllers
 *  the controller will implement this interface, which allows us to block access to other controller functionality since only this interface
 *  point is exposed
 */
class WizardPageValidator
{
public:
    virtual bool validate() = 0;
};
}
