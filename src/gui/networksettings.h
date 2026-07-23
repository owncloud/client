/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MIRALL_NETWORKSETTINGS_H
#define MIRALL_NETWORKSETTINGS_H

#include "libsync/creds/credentialmanager.h"

#include <QWidget>


namespace OCC {

namespace Ui {
    class NetworkSettings;
}

/**
 * @brief The NetworkSettings class
 * @ingroup gui
 */
class NetworkSettings : public QWidget
{
    Q_OBJECT

public:
    explicit NetworkSettings(QWidget *parent = nullptr);
    ~NetworkSettings() override;

private Q_SLOTS:
    void saveProxySettings();
    void saveMeteredSettings();

    /// Red marking of host field if empty and enabled
    void checkEmptyProxyHost();

    void checkAccountLocalhost();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void loadProxySettings();
    void removeBWLimitSettings();
    void loadMeteredSettings();
    CredentialManager *_credentialManager;

    Ui::NetworkSettings *_ui;
};


} // namespace OCC
#endif // MIRALL_NETWORKSETTINGS_H
