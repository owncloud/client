/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
 * Copyright (C) by Krzesimir Nowak <krzesimir@endocode.com>
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

#include <QDir>
#include <QFileDialog>
#include <QUrl>
#include <QTimer>
#include <QPushButton>
#include <QMessageBox>
#include <QMenu>
#include <QSsl>
#include <QSslCertificate>
#include <QNetworkAccessManager>
#include <QBuffer>

#include "QProgressIndicator.h"

#include "guiutility.h"
#include "wizard/owncloudwizardcommon.h"
#include "wizard/owncloudsetuppage.h"
#include "theme.h"
#include "account.h"

#include "ui_owncloudsetuppage.h"

namespace OCC {

OwncloudSetupPage::OwncloudSetupPage(OwncloudWizard *parent)
    : AbstractWizardPage(parent)
    , _ui(new Ui_OwncloudSetupPage)
{
    _ui->setupUi(this);

    Theme *theme = Theme::instance();
    setTitle(WizardCommon::titleTemplate().arg(tr("Connect to %1").arg(theme->appNameGUI())));
    setSubTitle(WizardCommon::subTitleTemplate().arg(tr("Setup %1 server").arg(theme->appNameGUI())));

    if (theme->overrideServerUrlV2().isEmpty()) {
        _ui->leUrl->setPostfix(theme->wizardUrlPostfix());
        _ui->leUrl->setPlaceholderText(theme->wizardUrlHint());
    } else {
        _ui->leUrl->setEnabled(false);
    }


    registerField(QLatin1String("OCUrl*"), _ui->leUrl);

    slotUrlChanged(QString()); // don't jitter UI
    connect(_ui->leUrl, &QLineEdit::textChanged, this, &OwncloudSetupPage::slotUrlChanged);
    connect(_ui->leUrl, &QLineEdit::editingFinished, this, &OwncloudSetupPage::slotUrlEditFinished);
}

void OwncloudSetupPage::setServerUrl(const QString &newUrl)
{
    _oCUrl = newUrl;
    if (_oCUrl.isEmpty()) {
        _ui->leUrl->clear();
        return;
    }

    _ui->leUrl->setText(_oCUrl);
}

// slot hit from textChanged of the url entry field.
void OwncloudSetupPage::slotUrlChanged(const QString &url)
{
    QString newUrl = url;
    if (url.endsWith("index.php")) {
        newUrl.chop(9);
    }
    if (owncloudWizard()->account()) {
        QString webDavPath = owncloudWizard()->account()->davPath();
        if (url.endsWith(webDavPath)) {
            newUrl.chop(webDavPath.length());
        }
        if (webDavPath.endsWith(QLatin1Char('/'))) {
            webDavPath.chop(1); // cut off the slash
            if (url.endsWith(webDavPath)) {
                newUrl.chop(webDavPath.length());
            }
        }
    }
    if (newUrl != url) {
        _ui->leUrl->setText(newUrl);
    }

    if (!url.startsWith(QLatin1String("https://"))) {
        _ui->urlLabel->setPixmap(Utility::getCoreIcon(QStringLiteral("lock-http")).pixmap(_ui->urlLabel->size()));
        _ui->urlLabel->setToolTip(tr("This url is NOT secure as it is not encrypted.\n"
                                     "It is not advisable to use it."));
    } else {
        _ui->urlLabel->setPixmap(Utility::getCoreIcon(QStringLiteral("lock-https")).pixmap(_ui->urlLabel->size()));
        _ui->urlLabel->setToolTip(tr("This url is secure. You can use it."));
    }
}

void OwncloudSetupPage::slotUrlEditFinished()
{
    QString url = _ui->leUrl->fullText();
    if (QUrl(url).isRelative() && !url.isEmpty()) {
        // no scheme defined, set one
        url.prepend("https://");
        _ui->leUrl->setFullText(url);
    }
}

bool OwncloudSetupPage::isComplete() const
{
    return !_ui->leUrl->text().isEmpty();
}

void OwncloudSetupPage::initializePage()
{
    WizardCommon::initErrorLabel(_ui->errorLabel);

    QAbstractButton *nextButton = wizard()->button(QWizard::NextButton);
    QPushButton *pushButton = qobject_cast<QPushButton *>(nextButton);
    if (pushButton)
        pushButton->setDefault(true);

    // If url is overriden by theme, it's already set and
    // we just check the server type and switch to second page
    // immediately.
    if (Theme::instance()->overrideServerUrlV2().isEmpty()) {
        _ui->leUrl->setFocus();
    } else {
        setCommitPage(true);
        // Hack: setCommitPage() changes caption, but after an error this page could still be visible
        setButtonText(QWizard::CommitButton, tr("&Next >"));
        if (validatePage()) {
            QTimer::singleShot(0, owncloudWizard(), &OwncloudWizard::next);
        }
    }
}

QString OwncloudSetupPage::url() const
{
    QString url = _ui->leUrl->fullText().simplified();
    return url;
}

bool OwncloudSetupPage::validatePage()
{
    slotUrlEditFinished();
    QString u = url();
    QUrl qurl(u);
    if (!qurl.isValid() || qurl.host().isEmpty()) {
        setErrorString(tr("Invalid URL"));
        return false;
    }
    owncloudWizard()->account()->setUrl(qurl);
    setErrorString(QString());
    return true;
}

void OwncloudSetupPage::setErrorString(const QString &err)
{
    if (err.isEmpty()) {
        _ui->errorLabel->setVisible(false);
    } else {
        _ui->errorLabel->setVisible(true);
        _ui->errorLabel->setText(err);
    }
    emit completeChanged();
}

OwncloudSetupPage::~OwncloudSetupPage()
{
}

} // namespace OCC
