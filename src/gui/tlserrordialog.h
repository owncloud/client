/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QDialog>
#include <QNetworkReply>

namespace OCC {

namespace Ui {
    class TlsErrorDialog;
}

class TlsErrorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TlsErrorDialog(const QList<QSslError> &sslErrors, const QString &host, QWidget *parent);
    ~TlsErrorDialog() override;

private:
    static QString describeCertificateHtml(const QSslCertificate &certificate);

    Ui::TlsErrorDialog *_ui;
};

}
