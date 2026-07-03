/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SPARKLEUPDATER_H
#define SPARKLEUPDATER_H

#include "updater/updater.h"

#include <QObject>

namespace OCC {

class SparkleUpdater : public Updater
{
    Q_OBJECT
public:
    SparkleUpdater(const QUrl &appCastUrl);
    ~SparkleUpdater() override;

    void setUpdateUrl(const QUrl &url);

    // unused in this updater
    void checkForUpdate() override;
    void backgroundCheckForUpdate() override;

    QString statusString();

private:
    class Private;
    Private *d;
};

} // namespace OCC

#endif // SPARKLEUPDATER_H
