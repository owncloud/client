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

#include "buttondelegate.h"
#include "common/utility.h"
#include "commonstrings.h"

#include "iconresources.h"

#include <QAbstractItemView>
#include <QEvent>
#include <QPainter>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStyleOptionButton>
#include <QTreeView>
namespace OCC {

ButtonDelegate::ButtonDelegate(QAbstractItemView *parent)
    : QItemDelegate{parent}
{
    // we can't really get the "right" parent here, and reusing the button is simpler and I'd guess slightly more efficient
    // than creating it over and over in createEditor.
    // note we update the widget parent in the first call to createEditor as that passes the correct parent
    // Not a leak!
    _button = new QPushButton();

    // on mac set the button to flat to get rid of crazy attempt to make it look "3d" or something
    if (Utility::isMac())
        _button->setFlat(true);

    // this is so shady: if I set the icon to 24x24 it still comes out at around 18x18
    // note the button height is actually 32 so I don't understand what the issue is if it's 24x24.
    // the target size could only be identified by trial and error so far.
    // To get a more robust impl, the only option I have found for getting the actual size of the button icon (maybe!)
    // requires getting it from the style option in play, which needs a call button->initializeStyleOption.
    // This function is protected so I'm not going crazy with that yet.
    // so far this impl works on both win and mac so I'm leaving it with the "hack" for now.
    QIcon elipsesIcon = IconResources::getCoreIcon("more").pixmap(_targetIconSize, _targetIconSize);
    _button->setIcon(elipsesIcon);
    _button->setObjectName("buttonDelegateButton");
    _button->setFocusPolicy(Qt::StrongFocus);
    _button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _button->setStyleSheet("QPushButton::menu-indicator{width:0px;}");

    _button->setAccessibleName(tr("%1 options button").arg(CommonStrings::capSpace()));
    _button->setAccessibleDescription(tr("Menu button with %1 options. Use the space key to show the menu").arg(CommonStrings::space()));
}

void ButtonDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    QStyleOptionButton buttonStyle;
    opt.palette.setColor(QPalette::Highlight, buttonStyle.palette.color(QPalette::Highlight));
    drawBackground(painter, opt, index);

    painter->save();

    // the test here for painting the button placeholder:
    // this is a top level row (invalid parent index)
    // AND
    // the button is not visible OR it's visible but somewhere else, most likely in another row
    // the idea is we *don't* want to paint the placeholder if the button is actually there, as it bleeds through
    // (button->setAutoFillBackground(true) is not an option as it has undesired side effects)
    if (!index.parent().isValid() && (!_button->isVisible() || !option.rect.contains(_button->pos()))) {
        int xpos = option.rect.left() + (option.rect.width() - _targetIconSize) / 2;
        int ypos = option.rect.top() + (option.rect.height() - _targetIconSize) / 2;
        QPixmap ellipses = IconResources::getCoreIcon("more").pixmap(_targetIconSize, _targetIconSize);
        QRect target(xpos, ypos, _targetIconSize, _targetIconSize);
        painter->drawPixmap(target, ellipses);
    }

    painter->setPen(QPen(QBrush("#807F7F7F"), 1));
    QRect r = option.rect;
    r.setRight(r.right() - 10);
    painter->drawLine(r.bottomLeft(), r.bottomRight());
    painter->restore();
}

QSize ButtonDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize baseSize = QItemDelegate::sizeHint(option, index);
    int width = _button->sizeHint().width();
    return QSize(width + 20, baseSize.height());
}

QWidget *ButtonDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    if (_button->parentWidget() != parent)
        // the parent is actually the scroll area viewport. don't ask :D
        _button->setParent(parent);
    return _button;
}


void ButtonDelegate::destroyEditor(QWidget *widget, const QModelIndex &index) const
{
    Q_UNUSED(widget);
    Q_UNUSED(index);
    // base impl calls delete later on the editor widget.
    // but we don't really want or need to delete the button, just re-use it. it should go away naturally when the
    // viewport is deleted, as we reparent to that in createEditor.
    // this is all pretty weird but logical if you investigate it ;)
}

void ButtonDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    Q_ASSERT(editor == _button);

    int xOffset = (option.rect.width() - _button->width()) / 2;
    int yOffset = (option.rect.height() - _button->height()) / 2;
    _button->move(option.rect.left() + xOffset, option.rect.top() + yOffset);
}

bool ButtonDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    // implement the menu auto-pop when clicking the delegate directly
    // super important! we can't use MouseButtonPress or the menu doesn't pop when eg the tree is not already focused
    // no idea why this is but release is the way to go, 100%
    if (event->type() == QEvent::MouseButtonRelease) {
        QAbstractItemView *view = qobject_cast<QAbstractItemView *>(parent());
        if (view) {
            QModelIndex current = view->currentIndex();
            if (current != index)
                view->setCurrentIndex(index);
            _button->setVisible(true);
            _button->showMenu();
            return true;
        }
    }
    return QItemDelegate::editorEvent(event, model, option, index);
}

void ButtonDelegate::setMenu(QMenu *menu)
{
    _button->setMenu(menu);
}

}
