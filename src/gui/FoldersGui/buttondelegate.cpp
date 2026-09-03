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

ButtonDelegate::ButtonDelegate(const QString &text, QAbstractItemView *parent)
    : QItemDelegate{parent}
    , _buttonText(text)
{
    // note we will update the widget parent in the first createEditor as that passes the correct parent for the pop
    // we can't really get the "right" parent here, and reusing the button is simpler and I'd guess slightly more efficient
    // than creating it over and over in create editor.

    //  _button = new QPushButton(_buttonText);
    // this is so shady: if I set the icon to 24x24 it still comes out at around 18x18
    // note the button height is actually 32 so I don't understand what the issue is.
    // this can only be identified by trial and error as the pixmap set on the button knows it's size (whatever I give it),
    // but it does not match the de facto painted size in the button.
    // I can't find the prop for how that works so will go with this for now
    // True test is whether it also works on windows. I expect it does not.
    QIcon elipsesIcon = IconResources::getCoreIcon("more").pixmap(18, 18);
    _button = new QPushButton("");
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
    if (!index.parent().isValid() && (!_button->isVisible() || !option.rect.contains(_button->pos()))) {
        // for reasons I can't even guess, the icon on the button is smaller than the requested 24x24.
        // so eyeball and hardcode the placeholder size so it's not too big.
        int placeholderSize = 18;
        /*   QStyleOptionButton buttonStyle;
           _button->initStyleOption(&buttonStyle); -> nope! this is protected, naturally.
           QSize optionSize = buttonStyle.iconSize; // this is -1, -1
           // qDebug() << "option size = " << optionSize;
           // option.icon.actualSize(optionSize);
           qDebug() << "option size = " << optionSize;
   */
        int xpos = option.rect.left() + (option.rect.width() - placeholderSize) / 2;
        int ypos = option.rect.top() + (option.rect.height() - placeholderSize) / 2;
        QPixmap ellipses = IconResources::getCoreIcon("more").pixmap(placeholderSize, placeholderSize);
        QRect target(xpos, ypos, placeholderSize, placeholderSize);
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
