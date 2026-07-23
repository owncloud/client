/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "ocsynclib.h"
#include <QObject>

namespace OCC {

class OCSYNC_EXPORT PluginFactory
{
public:
    virtual ~PluginFactory();
    virtual QObject* create(QObject* parent) = 0;
};

template<class PluginClass>
class DefaultPluginFactory : public PluginFactory
{
public:
    QObject* create(QObject *parent) override
    {
        return new PluginClass(parent);
    }
};

/// Return the expected name of a plugin, for use with QPluginLoader
QString pluginFileName(const QString &type, const QString &name);

}

Q_DECLARE_INTERFACE(OCC::PluginFactory, "org.owncloud.PluginFactory")
