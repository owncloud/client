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

#include "folderitemdelegate.h"

#include "folderitem.h"
#include <qpainter.h>

namespace OCC {
FolderItemDelegate::FolderItemDelegate(QObject *parent)
    : QItemDelegate{parent}
{
}

void FolderItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = setOptions(index, option);
    drawBackground(painter, option, index);

    //  QSize iconSize = opt.decorationSize; comes back as 16x16 - I think this options thing is fairly useless aside from providing the
    // basic rect we have to paint into for the item, which is based on size

    QPixmap decoration = index.data(Qt::DecorationRole).value<QIcon>().pixmap(option.rect.height(), option.rect.height());
    // see the d
    //  Q_ASSERT(!decoration.isNull());
    QRect iconRect(option.rect.topLeft(), decoration.size());
    drawDecoration(painter, option, iconRect, decoration);
    painter->save();
    painter->drawText(QPoint(iconRect.right(), iconRect.top()), index.data(Qt::DisplayRole).toString());
    painter->restore();
    // do the custom text painting using the painter
    drawFocus(painter, option, option.rect);
}

QSize FolderItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize defaultSize = QItemDelegate::sizeHint(option, index);
    // this is super optimistic :D
    defaultSize.setHeight(defaultSize.height() * 2);
    return defaultSize;
}

}
