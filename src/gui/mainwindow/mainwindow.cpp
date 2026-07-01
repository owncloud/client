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

#include "mainwindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>

#include <QMenu>

#include "configfile.h"
#include "modalwrapperwidget.h"
#include "theme.h"

#ifdef Q_OS_MAC
#include "settingsdialog_mac.h"
void setActivationPolicy(ActivationPolicy policy);
#endif

namespace OCC {
MainWindow::MainWindow()
    : QMainWindow{nullptr}
{
    // required as group for saveGeometry call. the original name is "Settings" so we should factor this in
    // and make sure we either accept that previous geometry is lost between upgrade/downgrades OR put the name back to Settings
    // which imo is a terrible name but we could save the rename til next major version.
    // in reality the stored geometry is just the size afaik but will double check. If we had user editable docking positions,
    // subwidgets that can run along side the main window, etc. the stored geometry would be more important for user happiness.
    setObjectName(QStringLiteral("MainWindow"));
    setWindowTitle(Theme::instance()->appNameGUI());

    // People perceive this as a Window, so also make Ctrl+W work
    addAction(tr("Hide"), Qt::CTRL | Qt::Key_W, this, &MainWindow::hide);

    ConfigFile().restoreGeometry(this);

#ifdef Q_OS_MAC
    setActivationPolicy(ActivationPolicy::Accessory);
#endif

    buildWindow();
}

void MainWindow::buildWindow()
{
    // clang is complaining that call to minimumSizeHint bypasses virtual dispatch, but I'm not seeing any problem.
    // the link to docs for that warning = 404 :D
    // regardless, the evaluation is misleading as according to c++ standard, calling a virtual function in a ctr is guaranteed to
    // call the override *for the class being constructed*. That is what we want so there is no problem here.
    // this is part of the reason we have tidy turned off - it's a bit too touchy.
    // question is, why is tidy butting in if we have it turned off? Figure this out later.
    setMinimumSize(minimumSizeHint());
    QSize iconsSize(24, 24);

    _actionGroup = new QActionGroup(this);
    _actionGroup->setExclusive(true);


    _toolbar = new QToolBar(this);
    _toolbar->setObjectName("mainWindowToolbar");
    // the height is in play if the toolbar is vertically oriented
    // not sure what the default separator width is, but without setting this style sheet
    // the space "around" the separator line itself was visibly "extra" if you eg select the activity button
    // which makes the button "edge" clear. Anyway without this fix, the  ... button looked really weird
    // looked like it was pushed too far to the right with a larger gap on the left side vs the right
    // this change helped a lot.
    _toolbar->setStyleSheet("QToolBar::Separator { width: 1px; height: 1px; }");
    _toolbar->setIconSize(iconsSize);
    _toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    _toolbar->setMovable(false);
    _toolbar->setAccessibleDescription(tr("Main toolbar for the application"));
    _toolbar->setAccessibleName(tr("Main toolbar"));
    addToolBar(Qt::TopToolBarArea, _toolbar);


    QWidget *toolbarStretch = new QWidget(this);
    toolbarStretch->setFocusPolicy(Qt::NoFocus);
    // who knows, maybe someday we use the toolbar vertically
    toolbarStretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _stretchAction = _toolbar->addWidget(toolbarStretch);

    // need to stash the separator action so we can insert other actions before it, ie after the stretch section
    _separatorAction = _toolbar->addSeparator();

    QAction *moreAction = new QAction(tr("More"), this);
    moreAction->setObjectName("moreAction");
    moreAction->setIcon(Resources::getCoreIcon("more"));
    moreAction->setToolTip(tr("More"));
    _toolbar->addAction(moreAction);
    _moreButton = qobject_cast<QToolButton *>(_toolbar->widgetForAction(moreAction));
    _moreButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    _moreButton->setFocusPolicy(Qt::StrongFocus);
    if (Utility::isMac()) {
        // does not work for QToolButton:
        // button->setAttribute(Qt::WA_MacShowFocusRect, true);
        // so add it to the style sheet
        _moreButton->setStyleSheet("QToolButton:focus { border: 2px solid palette(highlight); }"
                                   "QToolButton::menu-indicator { image: none; }");
    } else {
        _moreButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");
    }

    _moreButton->setPopupMode(QToolButton::InstantPopup);
    // QToolButtons have default fixed size, shaped to their content (watch out if it's text only, especially!).
    // In this toolbar all action buttons have icon+text which makes them effectively "taller" than the more button so there is
    // dead space which doesn't accept clicks above and below the more icon
    // So, make the more button expand vertically so you can click anywhere in the area, just like all the others.
    _moreButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    _widgetStack = new QStackedWidget(this);
    setCentralWidget(_widgetStack);
}

void MainWindow::setMoreMenuActions(const QList<QAction *> &actions)
{
    QMenu *moreMenu = _moreButton->menu();
    if (moreMenu)
        moreMenu->clear();
    else
        moreMenu = new QMenu(_moreButton);
    moreMenu->addActions(actions);
    _moreButton->setMenu(moreMenu);
}

void MainWindow::startModal()
{
    _toolbar->setEnabled(false);
}

void MainWindow::stopModal()
{
    _toolbar->setEnabled(true);
}

void MainWindow::showModalWidget(ModalWrapperWidget *w)
{
    // ownCloudGui::raise();
    // not sure if we should have an assert here but for real, the incoming modal widget should
    // never be in the current stack
    Q_ASSERT(_widgetStack->indexOf(w) == -1);

    connect(w, &ModalWrapperWidget::finished, this, &MainWindow::endModalWidget);
    _widgetStack->addWidget(w);
    _widgetStack->setCurrentWidget(w);
    startModal();
}

void MainWindow::endModalWidget()
{
    QWidget *target = qobject_cast<QWidget *>(sender());
    if (target == nullptr)
        return;

    if (_widgetStack->indexOf(target) >= 0)
        _widgetStack->removeWidget(target);
    target->deleteLater();
    stopModal();
    // ensure the current stack widget matches the currently "checked" action in the toolbar
    _actionGroup->checkedAction()->toggled(true);
}

void MainWindow::addGeneralAction(QAction *action)
{
    _toolbar->insertAction(_separatorAction, action);
    configureAction(action);
    updateFocusChain();
}

void MainWindow::addAccountAction(QAction *action)
{
    _toolbar->insertAction(_stretchAction, action);
    configureAction(action);
    updateFocusChain();
}

void MainWindow::configureAction(QAction *action)
{
    QWidget *widget = action->data().value<QWidget *>();
    if (widget) {
        QWidget *button = _toolbar->widgetForAction(action);
        Q_ASSERT(button);
        // todo: this does not seem to be working on the account buttons?! It only sets focus on click
        // when the button is already selected. for the error and activity buttons, it sets focus on first
        // click along with selecting the button. I do not get it.
        button->setFocusPolicy(Qt::StrongFocus);
        if (Utility::isMac()) {
            // does not work for QToolButton:
            // button->setAttribute(Qt::WA_MacShowFocusRect, true);
            button->setStyleSheet("QToolButton:focus { border: 2px solid palette(highlight); }");
        }

        connect(action, &QAction::toggled, this, &MainWindow::onViewActionTriggered);
        _actionGroup->addAction(action);
        _widgetStack->addWidget(widget);
    }
}
void MainWindow::removeAction(QAction *action)
{
    QWidget *widget = action->data().value<QWidget *>();
    if (widget) {
        action->disconnect();
        _actionGroup->removeAction(action);
        _toolbar->removeAction(action);
        _widgetStack->removeWidget(widget);
    }
}

void MainWindow::onViewActionTriggered(bool selected)
{
    if (!selected)
        return;

    QAction *sourceAction = qobject_cast<QAction *>(sender());
    if (sourceAction) {
        QWidget *widget = sourceAction->data().value<QWidget *>();
        if (widget)
            _widgetStack->setCurrentWidget(widget);
    }
}

void MainWindow::updateFocusChain()
{
    const auto actions = _toolbar->actions();
    if (actions.isEmpty())
        return;

    QWidget *prevItem = nullptr;
    for (QAction *action : actions) {
        if (action->objectName().isEmpty())
            continue;
        if (QWidget *widget = _toolbar->widgetForAction(action)) {
            if (widget->focusPolicy() == Qt::NoFocus)
                continue;
            if (prevItem == nullptr) {
                prevItem = widget;
                continue;
            }
            _toolbar->setTabOrder(prevItem, widget);
            prevItem = widget;
        }
    }
}

QSize MainWindow::minimumSizeHint() const
{
    const QSize min{800, 700}; // When changing this, please check macOS: widgets there have larger insets, so they take up more space.
    const auto screen = windowHandle() ? windowHandle()->screen() : QApplication::screenAt(QCursor::pos());
    if (screen) {
        const auto availableSize = screen->availableSize();
        if (availableSize.isValid()) {
            // Assume we can use at least 90% of the screen, if the screen is smaller than 800x700 pixels.
            //
            // Note: this means that the wizards have even less space: with the style we use, the
            // wizard tries to fit inside the window. So, if this is a common case that users have
            // such small screens, and the contents of the wizard screen are squashed together (or
            // not shown due to lack of space), we should consider putting that content in a
            // scroll-view.
            return min.boundedTo(availableSize * 0.9);
        }
    }
    return min;
}

void MainWindow::setVisible(bool visible)
{
    if (!visible) {
        ConfigFile cfg;
        cfg.saveGeometry(this);
    }

#ifdef Q_OS_MAC
    if (visible) {
        setActivationPolicy(ActivationPolicy::Regular);
    } else {
        setActivationPolicy(ActivationPolicy::Accessory);
    }
#endif

    QMainWindow::setVisible(visible);
}
}
