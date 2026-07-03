/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QString>

#include <updater/ocupdater.h>

namespace OCC {

/**
 * @brief AppImage Updater using AppImageUpdate
 * @ingroup gui
 */
class AppImageUpdater : public OCUpdater
{
    Q_OBJECT

public:
    explicit AppImageUpdater(const QUrl &url);
    void backgroundCheckForUpdate() override;

private:
    void versionInfoArrived(const UpdateInfo &succeeded) override;
};

} // namespace OCC
