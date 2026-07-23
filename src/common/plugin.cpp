/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "plugin.h"

namespace OCC {

PluginFactory::~PluginFactory() = default;

QString pluginFileName(const QString &type, const QString &name)
{
    return QStringLiteral("ownCloud_%2_%3").arg(type, name);
}

}
