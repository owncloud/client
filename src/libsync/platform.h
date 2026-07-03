/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <QCoreApplication>
#include <QString>

#include <memory>

#include "owncloudlib.h"

namespace OCC {

/**
 * @brief The Platform is the baseclass for all platform classes, which in turn implement platform
 *        specific functionality for the GUI.
 */
class OWNCLOUDSYNC_EXPORT Platform : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    static std::unique_ptr<Platform> create();

    virtual void migrate();

    virtual void setApplication(QCoreApplication *application);

    virtual void startServices();

Q_SIGNALS:
    void requestAttention();
};

} // OCC namespace

#endif // PLATFORM_H
