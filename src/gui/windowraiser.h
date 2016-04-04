/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#ifndef WINDOWRAISER_H
#define WINDOWRAISER_H

#include "systray.h"

class QWidget;

namespace OCC {


/**
 * @brief The WindowRaiser class
 * @ingroup gui
 */
class WindowRaiser
{
public:

    static void raiseDialog(QWidget *raiseWidget);

private:
    explicit WindowRaiser();
};

} // namespace OCC

#endif // WINDOWRAISER_H
