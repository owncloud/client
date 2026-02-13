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

#include <QTreeView>


namespace OCC {
FolderItemDelegate::FolderItemDelegate(QObject *parent)
    : QItemDelegate{parent}
{
}

void FolderItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // this is hopefully temporary - the idea for now is if we are painting an error row, just incorporate a separate paint into this delegate
    if (index.parent().isValid()) {
        paintError(painter, option, index);
        return;
    }

    // todo: bring back the dynamic highlight color setting which was moved to the tree palette, and make sure to paint the background for the whole
    // row -> the widget width - to ensure the area where the expander appears is the same color as the items. the expander bit is not covered
    // by the delegate.
    // basically make the option rect for the background the full width of the widget. YES this will overlap with the button delegate area so it may
    // not work. Alternately, cast the option.widget to qtreeview and ask for the indentation, then extend the option rect in that direction? I actually don't
    // think that will work as it would require setting negative x value :/ FUCKY FUCK FUCK

    drawBackground(painter, option, index);

    // calculate the line heights using current font - yes again, as we can't "store" the sizes calculated in sizeHint :/
    int firstLineHeight = 0;
    int secondLineHeight = 0;
    calculateLineHeights(option, firstLineHeight, secondLineHeight);

    int paintableHeight = firstLineHeight + secondLineHeight + _lineSpacing;

    QRect paintableArea(option.rect.left() + _cellBorder, option.rect.top() + _cellBorder, option.rect.width() - 2 * _cellBorder, paintableHeight);
    // we always fit the icon into this area:
    QRect iconAreaRect(paintableArea.topLeft(), QSize(paintableHeight, paintableHeight));

    QIcon ico = index.data(Qt::DecorationRole).value<QIcon>();
    QPixmap decoration = ico.pixmap(paintableHeight, paintableHeight);
    QSize actualIconSize = decoration.deviceIndependentSize().toSize();

    int verticalOffset = 0;
    int horizontalOffset = 0;
    if (actualIconSize.height() != actualIconSize.width()) {
        if (actualIconSize.height() < actualIconSize.width())
            verticalOffset = (iconAreaRect.height() - actualIconSize.height()) / 2;
        else
            horizontalOffset = (iconAreaRect.width() - actualIconSize.width()) / 2;
    }
    QRect iconRect(paintableArea.left() + horizontalOffset, paintableArea.top() + verticalOffset, actualIconSize.width(), actualIconSize.height());

    // drawDecoration gives very very strange results on mac when the deviceIndependentSize is used - the paint location is way off, "up" and
    // to the left - it looks like it may be adjusting location using dpr? Anyway it works better if we just paint it ourselves.
    // drawDecoration(painter, option, iconRect, decoration);

    painter->drawPixmap(iconRect, decoration);

    painter->save();

    QFont f = option.font;
    f.setPixelSize(_namePixelSize); // eh?
    painter->setFont(f);

    // important point -> painter::drawText, when using just a point to position it, the y position is used as baseline. as that is super helpful?
    // so we have to set up a rect instead to make positioning easier:
    QRect firstLineRect(
        iconRect.right() + horizontalOffset + _cellBorder, paintableArea.top(), paintableArea.width() - iconAreaRect.width() - _cellBorder, firstLineHeight);
    QRect actualFirstLineRect;
    painter->drawText(firstLineRect, Qt::TextSingleLine, index.data(Qt::DisplayRole).toString(), &actualFirstLineRect);

    painter->setFont(option.font);
    QRect statusIconRect(iconRect.right() + _cellBorder, firstLineRect.bottom() + _lineSpacing, secondLineHeight, secondLineHeight);
    QIcon statusIcon = index.data(FolderItemRoles::StatusIconRole).value<QIcon>();
    painter->drawPixmap(statusIconRect, statusIcon.pixmap(statusIconRect.size()));
    QRect secondLineRect(statusIconRect.right() + _cellBorder, firstLineRect.bottom() + _lineSpacing,
        paintableArea.width() - iconRect.width() - _cellBorder - statusIconRect.width() - _cellBorder, secondLineHeight);
    painter->drawText(secondLineRect, Qt::TextSingleLine, index.data(FolderItemRoles::StatusStringRole).toString(), nullptr);

    painter->setPen(QPen(QBrush(_separatorColor), _cellSeparatorWidth));
    painter->drawLine(option.rect.left(), option.rect.bottom(), option.rect.right(), option.rect.bottom());

    painter->restore();
}

void FolderItemDelegate::paintError(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    QRect widgetRect = option.widget->rect();
    int fullWidth = option.widget->width();
    opt.rect.setWidth(fullWidth);
    drawBackground(painter, opt, index);
    // bump the error over to roughly align with text on item above. needs refinement but for first pass ok
    opt.rect.setLeft(opt.rect.left() + 18);
    // this works fine - if we need to make it more complicated, ie for multiline error or who knows what, do a more detailed paint late
    QItemDelegate::paint(painter, opt, index);

    painter->save();
    painter->setPen(QPen(QBrush(_separatorColor), _cellSeparatorWidth));
    painter->drawLine(_cellBorder, opt.rect.bottom(), widgetRect.right() - _cellBorder, opt.rect.bottom());

    painter->restore();
}

QSize FolderItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize defaultSize = QItemDelegate::sizeHint(option, index);
    int firstLineHeight = 0;
    int secondLineHeight = 0;
    calculateLineHeights(option, firstLineHeight, secondLineHeight);
    int totalHeight = 0;

    if (index.parent().isValid())
        totalHeight = _cellBorder * 2 + _cellSeparatorWidth + secondLineHeight;
    else
        totalHeight = _cellBorder * 2 + _cellSeparatorWidth + firstLineHeight + secondLineHeight + _lineSpacing;

    defaultSize.setHeight(totalHeight);
    return defaultSize;
}

void FolderItemDelegate::calculateLineHeights(const QStyleOptionViewItem &option, int &firstLineHeight, int &secondLineHeight) const
{
    QFont f1 = option.font;
    f1.setPixelSize(_namePixelSize);
    QFontMetrics metrics1(f1);
    firstLineHeight = metrics1.height();

    QFont f2 = option.font;
    QFontMetrics metrics2(f2);
    secondLineHeight = metrics2.height();
}
}
