/*
 * Copyright (C) Lisa Reese <lisa.reese@kiteworks.com>
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
#include "requestauthenticationwidget.h"

// #include "resources.h"
#include "template.h"
#include "theme.h"

#include <QBuffer>
#include <QClipboard>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QToolTip>
#include <QVBoxLayout>

namespace OCC {

RequestAuthenticationWidget::RequestAuthenticationWidget(QWidget *parent)
    : QWidget{parent}
{
    updateColors();

    QString appName = Theme::instance()->appNameGUI();

    QLabel *logoLabel = new QLabel({}, this);
    logoLabel->setPixmap(Theme::instance()->wizardHeaderLogo().pixmap(200, 100));
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    logoLabel->setAccessibleName(tr("Application Name Logo %1").arg(appName));

    QLabel *titleLabel = new QLabel(tr("Sign in required").arg(appName), this);
    QFont welcomeFont = titleLabel->font();
    welcomeFont.setWeight(QFont::DemiBold);
    welcomeFont.setPixelSize(20);
    titleLabel->setFont(welcomeFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QLabel *instructionLabel = new QLabel(tr("You may have been automatically disconnected due to a server issue or time out. Please sign in again."), this);
    QFont font = instructionLabel->font();
    font.setPixelSize(14);
    instructionLabel->setFont(font);
    instructionLabel->setWordWrap(true);
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    _urlField = new QLabel(this);
    _urlField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _urlField->setAccessibleName(tr("Sign in URL")); // Prevent screen readers from reading the full URL!!!
    _urlField->setAccessibleDescription(tr("To copy the sign in URL to the clipboard, use the copy button"));

    _copyButton = new QPushButton(_copyIcon, QString(), this);
    _copyButton->setFlat(true);
    _copyButton->setContentsMargins(0, 0, 0, 0);
    _copyButton->setFixedSize(_urlField->height(), _urlField->height());
    _copyButton->setAccessibleDescription(tr("Copy URL to sign in"));
    _copyButton->installEventFilter(this);
    connect(_copyButton, &QPushButton::clicked, this, &RequestAuthenticationWidget::onCopyUrl);

    _errorField = new QLabel(QString(), this);
    QPalette errorPalette = _errorField->palette();
    errorPalette.setColor(QPalette::Text, Qt::red);
    errorPalette.setColor(QPalette::WindowText, Qt::red);
    _errorField->setPalette(errorPalette);
    _errorField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _errorField->setWordWrap(true);
    _errorField->setAlignment(Qt::AlignLeft);
    _errorField->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);

    QLabel *footerLogoLabel = nullptr;
    if (!Theme::instance()->wizardFooterLogo().isNull()) {
        footerLogoLabel = new QLabel({}, this);
        footerLogoLabel->setPixmap(Theme::instance()->wizardFooterLogo().pixmap(100, 52));
        footerLogoLabel->setAlignment(Qt::AlignCenter);
        footerLogoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        footerLogoLabel->setAccessibleName(tr("Additional logo defined by the organization"));
    }

    _cancelButton = new QPushButton(tr("Stay logged out"), this);
    connect(_cancelButton, &QPushButton::clicked, this, &RequestAuthenticationWidget::stayLoggedOutClicked);
    _signInButton = new QPushButton(tr("Sign in"), this);
    connect(_signInButton, &QPushButton::clicked, this, &RequestAuthenticationWidget::connectClicked);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(50, 50, 50, 50);
    layout->setSpacing(12);
    layout->addStretch(1);
    layout->addWidget(logoLabel, Qt::AlignCenter);
    layout->addSpacing(16);
    layout->addWidget(titleLabel);
    layout->addWidget(instructionLabel, Qt::AlignCenter);

    QHBoxLayout *urlAreaLayout = new QHBoxLayout();
    urlAreaLayout->setContentsMargins(0, 0, 0, 0);
    urlAreaLayout->setSpacing(0);
    urlAreaLayout->addWidget(_urlField, Qt::AlignLeft);
    urlAreaLayout->addWidget(_copyButton);
    layout->addLayout(urlAreaLayout, Qt::AlignCenter);
    layout->addWidget(_errorField, Qt::AlignLeft);

    if (footerLogoLabel)
        layout->addWidget(footerLogoLabel, Qt::AlignCenter);
    layout->addStretch(1);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(_cancelButton);
    buttonLayout->addWidget(_signInButton);
    layout->addLayout(buttonLayout, Qt::AlignRight);

    setLayout(layout);

    // disable buttons until the url has been set
    _cancelButton->setEnabled(false);
    _signInButton->setEnabled(false);
}

void RequestAuthenticationWidget::setAuthUrl(const QString &url)
{
    // who knows.
    Q_ASSERT(!url.isEmpty());
    if (_authUrl != url) {
        _authUrl = url;
        QFontMetrics metrics(_urlField->font());
        QString elidedText = metrics.elidedText(_authUrl, Qt::ElideRight, _urlField->width());
        _urlField->setText(elidedText);
        // update the copy button tooltip
        onClipboardChanged();

        // and enable the buttons:
        _cancelButton->setEnabled(true);
        _signInButton->setEnabled(true);
    }
}

void RequestAuthenticationWidget::onClipboardChanged()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard->text() == _authUrl)
        _copyButton->setToolTip(tr("URL copied"));
    else
        _copyButton->setToolTip(tr("Copy URL"));
}

void RequestAuthenticationWidget::onCopyUrl()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(_authUrl);
}

void RequestAuthenticationWidget::updateColors()
{
    QPalette palette = this->palette();
    QColor themeBackground = Theme::instance()->wizardHeaderBackgroundColor();
    QColor themeForeground = Theme::instance()->wizardHeaderTitleColor();
    if (themeBackground.isValid()) {
        palette.setColor(QPalette::Window, themeBackground);
        palette.setColor(QPalette::Base, themeBackground);
    }
    if (themeForeground.isValid()) {
        palette.setColor(QPalette::WindowText, themeForeground);
        palette.setColor(QPalette::Text, themeForeground);
    }

    const QString iconPath = QStringLiteral(":/client/resources/core/copy.svg");
    const QString color = palette.color(QPalette::Text).name();
    QByteArray iconData = Resources::Template::renderTemplateFromFile(iconPath, {{QStringLiteral("color"), color}}).toUtf8();
    QBuffer buffer(&iconData);
    QImageReader iconReader(&buffer, "svg");
    _copyIcon = QIcon(QPixmap::fromImageReader(&iconReader));

    setPalette(palette);
    setAutoFillBackground(true);
}

void RequestAuthenticationWidget::setErrorMessage(const QString &error)
{
    _errorField->setText(error);
}

}
