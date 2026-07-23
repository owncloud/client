/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "settingsdialog_mac.h"
#include "common/utility.h"

#import <AppKit/AppKit.h>
#include <QDebug>

void setActivationPolicy(ActivationPolicy policy)
{
    NSApplicationActivationPolicy mode = NSApplicationActivationPolicyRegular;
    switch (policy) {
    case ActivationPolicy::Regular:
        mode = NSApplicationActivationPolicyRegular;
        break;
    case ActivationPolicy::Accessory:
        mode = NSApplicationActivationPolicyAccessory;
        break;
    case ActivationPolicy::Prohibited:
        mode = NSApplicationActivationPolicyProhibited;
        break;
    }

    if (mode != NSApp.activationPolicy) {
        if (![NSApp setActivationPolicy:mode]) {
            qWarning() << "setActivationPolicy" << static_cast<int>(policy) << "failed";
        }
    }
}
