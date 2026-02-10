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
    // we can't really get the "right" parent here, and reusing the button is simpler and I'd guess slightly more efficient
    // than creating it over and over in create editor.
    _button = new QPushButton(_buttonText);
    _button->setObjectName("buttonDelegateButton");
    // I think this only needs to be set in the underlying item.
    //_button->setAccessibleName("Folder options");
    //_button->setAccessibleDescription("Menu button with folder options. Hit the space key to auto-pop the menu");

    _button->setAutoFillBackground(true);
    // have to explicitly align center or the text goes left when a menu is added
    // also the font size is set based on the idea that the text is "..." - we may want to adapt this in future
    _button->setStyleSheet({"font: bold 18px; text-align:center;"});
}

void ButtonDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // we have to override the default selection color for item views which is "dark-blue"
    // use the default button highlight color instead to make it more cohesive
    // note we can't just set the palette on the tree view as that won't update automatically when system color changes :/
    QStyleOptionViewItem localOpt(option);
    QStyleOptionButton buttonStyle;
    localOpt.palette.setColor(QPalette::Highlight, buttonStyle.palette.color(QPalette::Highlight));
    drawBackground(painter, localOpt, index);

    painter->save();
    QFont f = painter->font();
    f.setBold(true);
    f.setPixelSize(18);
    painter->setFont(f);
    painter->drawText(option.rect, Qt::AlignCenter, _buttonText);
    painter->setPen(QPen(QBrush("#807F7F7F"), 1));
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    painter->restore();
}

QSize ButtonDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QItemDelegate::sizeHint(option, index);
    return QSize(75, size.height());
}

QWidget *ButtonDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    Q_UNUSED(option);

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
    // make the button just a bit smaller so you can really see it's a button and not just a full cell "block".
    QRect adaptedRect(option.rect);
    adaptedRect.adjust(5, 5, -5, -5);
    _button->setGeometry(adaptedRect);
}

void ButtonDelegate::setMenu(QMenu *menu)
{
    _button->setMenu(menu);
}

}
