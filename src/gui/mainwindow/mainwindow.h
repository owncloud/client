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

#include <QMainWindow>

class QAction;
class QToolBar;
class QToolButton;
class QStackedWidget;
class QActionGroup;

namespace OCC {

class ModalWrapperWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow();

    QSize minimumSizeHint() const override;

    void setVisible(bool visible) override;

    void setMoreMenuActions(const QList<QAction *> &actions);

    void showModalWidget(ModalWrapperWidget *w);

    void addAccountAction(QAction *action);
    void removeAction(QAction *action);

    void addGeneralAction(QAction *action);

private slots:
    void endModalWidget();
    void onViewActionTriggered(bool selected);

private:
    void buildWindow();
    // these just disable the main toolbar so the user can't do anything while a "modal" widget is shown
    // in main window stack. We are adjusting the behavior of account modal widgets to *not* disable other functions as it should
    // not cause any problems to eg switch to local/error activity view, or trigger one of the more menu actions while an account
    // is "waiting" for the user to complete something there.
    void startModal();
    void stopModal();
    void configureAction(QAction *action);
    void updateFocusChain();

    QToolBar *_toolbar = nullptr;
    QStackedWidget *_widgetStack = nullptr;
    QAction *_separatorAction = nullptr;
    QAction *_stretchAction = nullptr;
    QToolButton *_moreButton = nullptr;
    QActionGroup *_actionGroup = nullptr;
};
}
