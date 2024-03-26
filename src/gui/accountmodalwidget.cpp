/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
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

#include "accountmodalwidget.h"
#include "ui_accountmodalwidget.h"

#include "resources/resources.h"

#include <QMessageBox>
#include <QPushButton>
#include <QQuickWidget>

namespace OCC {

namespace {
    auto *createQmlWidget(const QUrl &url, const QList<QQmlContext::PropertyPair> &properties)
    {
        auto *widget = new QQuickWidget;
        widget->engine()->rootContext()->setContextProperties(properties);
        widget->engine()->addImageProvider(QStringLiteral("ownCloud"), new Resources::CoreImageProvider());
        widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
        widget->setSource(url);
        if (!widget->errors().isEmpty()) {
            auto box = new QMessageBox(QMessageBox::Critical, QStringLiteral("QML Error"), QDebug::toString(widget->errors()));
            box->setAttribute(Qt::WA_DeleteOnClose);
            box->exec();
            qFatal("A qml error occured %s", qPrintable(QDebug::toString(widget->errors())));
        }
        return widget;
    }
}

AccountModalWidget::AccountModalWidget(
    const QString &title, const QUrl &qmlSrc, const QList<QQmlContext::PropertyPair> &properties, const QList<Button> &buttons, QWidget *parent)
    : AccountModalWidget(title, createQmlWidget(qmlSrc, properties), buttons, parent)
{
}


AccountModalWidget::AccountModalWidget(const QString &title, QWidget *widget, const QList<Button> &buttons, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AccountModalWidget)
{
    ui->setupUi(this);
    ui->groupBox->setTitle(title);
    ui->groupBox->layout()->addWidget(widget);

    if (!buttons.isEmpty()) {
        ui->buttonBox->clear();
        for (auto [button, role] : buttons) {
            ui->buttonBox->addButton(button, role);
        }
    }

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this] {
        Q_EMIT accepted();
        Q_EMIT finished();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, [this] {
        Q_EMIT rejected();
        Q_EMIT finished();
    });

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this](auto *button) { _clickedButton = button; });
}

QDialogButtonBox *AccountModalWidget::buttons()
{
    return ui->buttonBox;
}

QAbstractButton *AccountModalWidget::clickedButton() const
{
    return _clickedButton;
}

} // OCC
