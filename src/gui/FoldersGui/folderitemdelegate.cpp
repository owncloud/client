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
#include <QPainter>


namespace OCC {
FolderItemDelegate::FolderItemDelegate(QObject *parent)
    : QItemDelegate{parent}
{
}

void FolderItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    drawBackground(painter, option, index);

    // calculate the line heights using current font - yes again, as we can't "store" the sizes calculated in sizeHint :/
    int firstLineHeight = 0;
    int secondLineHeight = 0;
    calculateLineHeights(option, firstLineHeight, secondLineHeight);

    QRect paintableArea(
        option.rect.left() + _cellBorder, option.rect.top() + _cellBorder, option.rect.width() - 2 * _cellBorder, firstLineHeight + secondLineHeight);
    QPixmap decoration = index.data(Qt::DecorationRole).value<QIcon>().pixmap(paintableArea.height(), paintableArea.height());
    QRect iconRect(paintableArea.topLeft(), decoration.size());
    drawDecoration(painter, option, iconRect, decoration);

    painter->save();

    QFont f = option.font;
    f.setPixelSize(14);
    painter->setFont(f);
    // inportant point -> painter::drawText, when using just a point to position it, the y position is used as baseline. as that is super helpful?
    // so we should set up a rect instead:
    QRect firstLineRect(iconRect.right(), iconRect.top(), paintableArea.width() - iconRect.width(), firstLineHeight);
    QRect actualFirstLineRect;
    painter->drawText(firstLineRect, Qt::TextSingleLine, index.data(Qt::DisplayRole).toString(), &actualFirstLineRect);

    painter->setFont(option.font);
    QRect statusIconRect(iconRect.right(), firstLineRect.bottom(), secondLineHeight, secondLineHeight);
    QIcon statusIcon = index.data(FolderItemRoles::StatusIconRole).value<QIcon>();
    painter->drawPixmap(statusIconRect, statusIcon.pixmap(statusIconRect.size()));
    QRect secondLineRect(
        statusIconRect.right() + 2, firstLineRect.bottom(), paintableArea.width() - iconRect.width() - statusIconRect.width() - 2, secondLineHeight);
    painter->drawText(secondLineRect, Qt::TextSingleLine, "Statuspqy", nullptr);

    painter->setPen(QPen(QBrush(_separatorColor), _cellSeparatorWidth));
    painter->drawLine(paintableArea.left(), option.rect.bottom(), paintableArea.right(), option.rect.bottom());
    painter->restore();

    // do the custom text painting using the painter
    drawFocus(painter, option, option.rect);
}

QSize FolderItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize defaultSize = QItemDelegate::sizeHint(option, index);
    int firstLineHeight = 0;
    int secondLineHeight = 0;
    calculateLineHeights(option, firstLineHeight, secondLineHeight);
    int totalHeight = _cellBorder * 2 + _cellSeparatorWidth + firstLineHeight + secondLineHeight;
    defaultSize.setHeight(totalHeight);
    return defaultSize;
}

void FolderItemDelegate::calculateLineHeights(const QStyleOptionViewItem &option, int &firstLineHeight, int &secondLineHeight) const
{
    QFont f1 = option.font;
    f1.setPixelSize(14);
    QFontMetrics metrics1(f1);
    firstLineHeight = metrics1.height();

    QFont f2 = option.font;
    QFontMetrics metrics2(f2);
    secondLineHeight = metrics2.height();
}
}
