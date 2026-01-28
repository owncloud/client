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

#include <QPainter>
#include <QPushButton>

namespace OCC {

ButtonDelegate::ButtonDelegate(QObject *parent)
    : QItemDelegate{parent}
{
    _button = new QPushButton("...");
}

void ButtonDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    drawBackground(painter, option, index);
    drawDisplay(painter, option, option.rect, "...");
    painter->save();
    // QRect dotsRect = option.rect;
    // painter->drawLine()
    painter->setPen(QPen(QBrush("#807F7F7F"), 1));
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    painter->restore();

    drawFocus(painter, option, option.rect);
}

QSize ButtonDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(75, option.rect.height());
}

QWidget *ButtonDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);

    if (_button->parent() != parent)
        // the parent is actually the scroll area viewport. don't ask :D
        _button->setParent(parent);
    _button->setGeometry(option.rect);
    // maybe we need to refresh the menu but I think the row will be auto-selected on any click, including in the button area
    return _button;
}

void ButtonDelegate::destroyEditor(QWidget *widget, const QModelIndex &index) const
{
    // base impl calls delete later on the editor widget.
    // but we don't want to delete the button, just re-use it. it should go away naturally when the
    // viewport is deleted.
}

void ButtonDelegate::setMenu(QMenu *menu)
{
    _button->setMenu(menu);
}


}
