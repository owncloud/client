/*
 * Copyright (C) by Lisa Reese <lisa.reese@kiteworks.com>
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


#include "accountview.h"
#include "ui_accountview.h"

#include <QMenu>

namespace OCC {

AccountView::AccountView(QWidget *parent)
    : QWidget(parent)
    , _ui(new Ui::AccountView)
{
    _ui->setupUi(this);
    _ui->connectionStatusLabel->clear();

    connect(_ui->stackedWidget, &QStackedWidget::currentChanged, this,
        [this] { _ui->manageAccountButton->setEnabled(_ui->stackedWidget->currentWidget() == _ui->accountFoldersView); });
    _ui->stackedWidget->setCurrentWidget(_ui->accountFoldersView);
}

AccountView::~AccountView()
{
    delete _ui;
}

void AccountView::setAccountMenuActions(QList<QAction *> actions)
{
    QMenu *menu = _ui->manageAccountButton->menu();
    if (!menu) {
        menu = new QMenu(this);
        menu->setObjectName("manageAccountMenu");
        menu->setAccessibleName(tr("Account options menu"));
        connect(menu, &QMenu::aboutToShow, this, &AccountView::requestMenuActionUpdate);
    } else {
        menu->clear();
    }

    menu->addActions(actions);
    if (_ui->manageAccountButton->menu() == nullptr)
        _ui->manageAccountButton->setMenu(menu);
}

void AccountView::setTopStackWidget(QWidget *widget)
{
    // should not contain this widget already
    Q_ASSERT(_ui->stackedWidget->indexOf(widget) < 0);

    _ui->stackedWidget->addWidget(widget);
    _ui->stackedWidget->setCurrentWidget(widget);
}

void AccountView::removeStackWidget(QWidget *widget)
{
    _ui->stackedWidget->removeWidget(widget);
}

AccountFoldersView *AccountView::foldersView()
{
    return _ui->accountFoldersView;
}

void AccountView::accountSettingUpChanged(bool settingUp)
{
    if (settingUp) {
        _ui->spinner->startAnimation();
        _ui->stackedWidget->setCurrentWidget(_ui->loadingPage);
    } else {
        _ui->spinner->stopAnimation();
        _ui->stackedWidget->setCurrentWidget(_ui->accountFoldersView);
    }
}

void AccountView::showEvent(QShowEvent *ev)
{
    Q_UNUSED(ev);
    _ui->manageAccountButton->setFocus();
}

void AccountView::setConnectionLabel(const QString &message, const QIcon &icon, QStringList errors)
{
    // I really see no point in these but they existed previously...eval usefulness later
    // oh wait...the "warning label" is actually the name of the STATUS ICON?!?!
    // if so this makes more sense. for goodness sake, let's rename that item asap :D
    _ui->warningLabel->setVisible(!icon.isNull());
    if (!icon.isNull()) {
        _ui->warningLabel->setPixmap(icon.pixmap(_ui->warningLabel->size()));
    }

    _ui->accountStatus->setVisible(!message.isEmpty());

    if (errors.isEmpty()) {
        _ui->connectionStatusLabel->setText(message);
        _ui->connectionStatusLabel->setToolTip(QString());
    } else {
        errors.prepend(message);
        const QString msg = errors.join(QLatin1String("\n"));
        _ui->connectionStatusLabel->setText(msg);
        _ui->connectionStatusLabel->setToolTip(QString());
    }
}

} // namespace OCC
