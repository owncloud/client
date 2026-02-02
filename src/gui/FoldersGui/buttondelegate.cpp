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

ButtonDelegate::ButtonDelegate(const QString &text, QObject *parent)
    : QItemDelegate{parent}
    , _buttonText(text)
{
    // note we will update the widget parent in the first createEditor as that passes the correct parent for the pop
    // trying to set this manually is error prone
    _button = new QPushButton(_buttonText);
    _button->setObjectName("buttonDelegateButton");
    _button->setAccessibleName("Folder options");
    _button->setAccessibleDescription("Menu button with folder options");

    _button->setAutoFillBackground(true);
    // have to explicitly align center or the text goes left when a menu is added
    // also the font size is set based on the idea that the text is "..." - we may want to adapt this in future
    _button->setStyleSheet({"font: bold 18px; text-align:center;"});
    _button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

void ButtonDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    drawBackground(painter, option, index);

    painter->save();
    QFont f = painter->font();
    f.setBold(true);
    f.setPixelSize(18);
    painter->setFont(f);
    painter->drawText(option.rect, Qt::AlignCenter, _buttonText);
    painter->setPen(QPen(QBrush("#807F7F7F"), 1));
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    painter->restore();

    drawFocus(painter, option, option.rect);
}

QSize ButtonDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QItemDelegate::sizeHint(option, index);
    return QSize(75, size.height());
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
