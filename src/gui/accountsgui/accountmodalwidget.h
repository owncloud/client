/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "gui/qmlutils.h"

#include <QDialogButtonBox>

namespace OCC {

namespace Ui {
    class AccountModalWidget;
}

// this class is basically a "wrapper" that hosts a widget/panel that we want to run in a modal concept.
// basically it adds a title to the given widget, and provides buttons (normally ok/cancel) that control the lifetime of
// the "modality"
// the class is used in the AccountView with account related stuff
// it's basically a reproduction of a modal dialog but it's embedded in the account view of the main window.
// it's also possible to use it to wrap a QDialog, such as the FolderWizard, in which case the dialog's accept/reject
// signals are taken over for the modal widget result
class AccountModalWidget : public QWidget
{
    Q_OBJECT
public:
    AccountModalWidget(const QString &title, QWidget *widget, QWidget *parent);
    AccountModalWidget(const QString &title, const QUrl &qmlSource, QObject *qmlContext, QWidget *parent);

    // if the widget is a QDialog, these functions silently do nothing (because the dialog buttons already exist)
    void setStandardButtons(QDialogButtonBox::StandardButtons buttons);
    QPushButton *addButton(const QString &text, QDialogButtonBox::ButtonRole role);

public Q_SLOTS:
    void accept();
    void reject();

Q_SIGNALS:
    void accepted();
    void rejected();
    void finished(OCC::AccountModalWidget *widget);

private:
    Ui::AccountModalWidget *ui;

    bool _widgetHasButtons = false;
};

} // OCC
