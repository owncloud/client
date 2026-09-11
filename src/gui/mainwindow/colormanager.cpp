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

#include "colormanager.h"

#include <QAction>
#include <QGuiApplication>

#include "resources/iconresources.h"

namespace OCC {

ColorManager::ColorManager(QObject *parent)
    : QObject{parent}
{
    connect(qGuiApp->styleHints(), &QStyleHints::colorSchemeChanged, this, &ColorManager::updateColorScheme);
}

void ColorManager::addActionIcon(QAction *action, const QString &iconName)
{
    connect(this, &ColorManager::refreshCoreIcons, action, [action, iconName]() { action->setIcon(IconResources::getCoreIcon(iconName)); });
}

void ColorManager::refresh()
{
    updateColorScheme(qGuiApp->styleHints()->colorScheme());
}

void ColorManager::updateColorScheme(Qt::ColorScheme colorScheme)
{
    Q_UNUSED(colorScheme);

    IconResources::handleSystemStyleChanged();

    emit refreshCoreIcons();
}
}
