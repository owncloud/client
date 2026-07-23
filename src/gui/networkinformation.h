/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "gui/owncloudguilib.h"

#include <QNetworkInformation>

namespace OCC {

/**
 * @brief Wrapper class for QNetworkInformation
 *
 * This class is used instead of QNetworkInformation so we do not need to check for an instance,
 * and to facilitate debugging by being able to force certain network states (i.e. captive portal).
 */
class OWNCLOUDGUI_EXPORT NetworkInformation : public QObject
{
    Q_OBJECT

public:
    static NetworkInformation *instance();

    bool isMetered();

    using Feature = QNetworkInformation::Feature;
    using Features = QNetworkInformation::Features;
    using Reachability = QNetworkInformation::Reachability;

    bool supports(Features features) const;

    bool isForcedCaptivePortal() const;
    void setForcedCaptivePortal(bool onoff);
    bool isBehindCaptivePortal() const;

Q_SIGNALS:
    void isMeteredChanged(bool isMetered);
    void reachabilityChanged(NetworkInformation::Reachability reachability);
    void isBehindCaptivePortalChanged(bool state);

private Q_SLOTS:
    void slotIsBehindCaptivePortalChanged(bool state);

private:
    NetworkInformation();

    static NetworkInformation *_instance;

    bool _forcedCaptivePortal = false;
};

}
