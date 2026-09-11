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

#include <QHash>
#include <QObject>
#include <QStyleHints>

class QAction;

namespace OCC {

class ColorManager : public QObject
{
    Q_OBJECT

public:
    explicit ColorManager(QObject *parent);

    void addActionIcon(QAction *action, const QString &iconName);
    void refresh();

signals:
    void refreshCoreIcons();

protected:
    void updateColorScheme(Qt::ColorScheme colorScheme);
};

}
