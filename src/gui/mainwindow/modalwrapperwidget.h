/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QWidget>

namespace OCC {

/**
 * @brief The ModalWrapperWidget class is used to show "child" widgets in the main window, in a quasi modal fashion, ie
 * the toolbar is disabled and you can't do anything in the main window until you finish interacting with the content.
 *
 * This impl eliminates the need for caller to manage the lifetime of the widget they want to run as the content widget is reparented
 * to the ModalWrapperWidget and the main window closes and deletes the wrapper when it receives the finished signal. It also eliminates
 * the need for the content widget to create and manage its own buttons. In short, this class ordinarily expects a simple "panel" widget,
 * and it adds a single "close" button below the content widget to dismiss the modal view.
 *
 * However, to support the updater, it is now also possible to pass a QDialog to the ctr in which case the dialog is used as-is
 * with its own buttons in place. In this case the dialog's finished signal is forwarded to ModalWrapperWidget::finished.
 *
 * You *must* use a wrapper widget if you want to use MainWindow::showModalWidget
 */
class ModalWrapperWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ModalWrapperWidget(QWidget *content, QWidget *parent);

signals:
    void finished();
};

}
