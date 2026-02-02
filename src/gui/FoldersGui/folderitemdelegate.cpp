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
    QRect iconRect(paintableArea.topLeft(), decoration.deviceIndependentSize().toSize()); // else the size is very wrong, given mac uses "special" dpi
    // drawDecoration gives very very strange results on mac when the deviceIndependentSize is used - the paint location is way off, "up" and
    // to the left - it looks like it may be adjusting location using dpr? Anyway it works better if we just paint it ourselves.
    // drawDecoration(painter, option, iconRect, decoration);
    painter->drawPixmap(iconRect, decoration);

    painter->save();

    QFont f = option.font;
    f.setPixelSize(14);
    painter->setFont(f);

    // inportant point -> painter::drawText, when using just a point to position it, the y position is used as baseline. as that is super helpful?
    // so we have to set up a rect instead to make positioning easier:
    QRect firstLineRect(iconRect.right() + _cellBorder, iconRect.top(), paintableArea.width() - iconRect.width() - _cellBorder, firstLineHeight);
    QRect actualFirstLineRect;
    painter->drawText(firstLineRect, Qt::TextSingleLine, index.data(Qt::DisplayRole).toString(), &actualFirstLineRect);

    painter->setFont(option.font);
    QRect statusIconRect(iconRect.right() + _cellBorder, firstLineRect.bottom(), secondLineHeight, secondLineHeight);
    QIcon statusIcon = index.data(FolderItemRoles::StatusIconRole).value<QIcon>();
    painter->drawPixmap(statusIconRect, statusIcon.pixmap(statusIconRect.size()));
    QRect secondLineRect(statusIconRect.right() + _cellBorder, firstLineRect.bottom(),
        paintableArea.width() - iconRect.width() - _cellBorder - statusIconRect.width() - _cellBorder, secondLineHeight);
    painter->drawText(secondLineRect, Qt::TextSingleLine, index.data(FolderItemRoles::StatusStringRole).toString(), nullptr);

    painter->setPen(QPen(QBrush(_separatorColor), _cellSeparatorWidth));
    painter->drawLine(option.rect.left(), option.rect.bottom(), option.rect.right(), option.rect.bottom());
    painter->restore();

    // do the custom text painting using the painter
    // drawFocus(painter, option, option.rect);
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
