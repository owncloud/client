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

#include "resources.h"
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
    _button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    // I think this only needs to be set in the underlying item.
    //_button->setAccessibleName("Folder options");
    //_button->setAccessibleDescription("Menu button with folder options. Hit the space key to auto-pop the menu");
}

void ButtonDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    QStyleOptionButton buttonStyle;
    opt.palette.setColor(QPalette::Highlight, buttonStyle.palette.color(QPalette::Highlight));
    drawBackground(painter, opt, index);

    painter->save();
    // this rigamarole is "needed" because if I set the button to autoFillBackground (which is super normal) there are weird artifacts of
    // the window color above and below the button. No idea. This is on mac at least. The fix is to not paint the text dots if the button
    // is showing on the current cell, as if we paint them there is bleed through visibility of the text under the button since we apparently
    // autoFillBackground has to be off :/
    if (!_button->isVisible() || !option.rect.contains(_button->pos())) {
        QFont f = painter->font();
        f.setBold(true);
        f.setPixelSize(18);
        painter->setFont(f);
        painter->drawText(option.rect, Qt::AlignCenter, _buttonText);
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

    int xOffset = (option.rect.width() - _button->width()) / 2;
    int yOffset = (option.rect.height() - _button->height()) / 2;
    _button->move(option.rect.left() + xOffset, option.rect.top() + yOffset);
}

void ButtonDelegate::setMenu(QMenu *menu)
{
    _button->setMenu(menu);
}

}
