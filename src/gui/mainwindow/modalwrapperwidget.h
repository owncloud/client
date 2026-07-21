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
