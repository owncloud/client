/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
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

#include "theme.h"

namespace OCC {

/**
 * @brief The ownCloudTheme class
 * @ingroup libsync
 */
class ownCloudTheme : public Theme
{
    Q_OBJECT
public:
    ownCloudTheme();
    QColor wizardHeaderBackgroundColor() const override;
    QColor wizardHeaderTitleColor() const override;
    QIcon wizardHeaderLogo() const override;
    QIcon aboutIcon() const override;
    bool moveToTrashDefaultValue() const override;
    bool allowSystemConfigOverrides() const override;
};
}
