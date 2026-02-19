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
    // if we want to make a dedicated delegate first check that setting the delegate per row overrides the column delegate per the docs. So far
    // many QAbstractItemView settings do not actually work with a tree view - heck, some tree view functions don't work!
    // so I am very wary about this and updating with a new error delegate is now much lower prio.
    if (index.parent().isValid()) {
        paintError(painter, option, index);
        return;
    }


    QStyleOptionViewItem opt(option);

    // this paints the full row color just fine, but unfortunately it seems to erase the expander icon which is presumably drawn by the tree.
    // this means we have to set the highlight color on the tree, too, but that looks icky when switching between dark and light modes
    // after the app is running. At least this only affects the indent area for now, where the expander is.
    // Until we have addressed todo #60 we simply have to live with this.
    /* int treeIndent = 0;
     const QTreeView *tree = qobject_cast<const QTreeView *>(option.widget);
     if (tree)
         treeIndent = tree->indentation();
     opt.rect.setX(opt.rect.X() - treeIndent);
*/

    QStyleOptionButton buttonStyle;
    opt.palette.setColor(QPalette::Highlight, buttonStyle.palette.color(QPalette::Highlight));
    drawBackground(painter, opt, index);

    // calculate the line heights using current font - yes again, as we can't "store" the sizes calculated in sizeHint :/
    int firstLineHeight = 0;
    int secondLineHeight = 0;
    calculateLineHeights(option, firstLineHeight, secondLineHeight);

    int paintableHeight = firstLineHeight + secondLineHeight + _lineSpacing;

    // we only apply the cell border on the left side to avoid having a duplicated gap between the text and button delegate
    QRect paintableArea(option.rect.left() + _cellBorder, option.rect.top() + _cellBorder, option.rect.width() - _cellBorder, paintableHeight);
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

    QRect iconRect(iconAreaRect.left() + horizontalOffset, iconAreaRect.top() + verticalOffset, actualIconSize.width(), actualIconSize.height());
    // drawDecoration gives very very strange results on mac when the deviceIndependentSize is used - the paint location is way off, "up" and
    // to the left - it looks like it may be adjusting location using dpr? Anyway it works better if we just paint it ourselves.
    // drawDecoration(painter, option, iconRect, decoration);

    painter->drawPixmap(iconRect, decoration);

    painter->save();

    QFont f = option.font;
    f.setPixelSize(_namePixelSize);
    painter->setFont(f);

    int nameWidth = paintableArea.width() - iconAreaRect.width() - _horizontalSpacing;
    QFontMetrics metrics(f);
    QString paintableString = metrics.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, nameWidth);
    QRect firstLineRect(iconAreaRect.right() + _horizontalSpacing, paintableArea.top(), nameWidth, firstLineHeight);

    // important point -> painter::drawText, when using just a point to position it, the y position is used as baseline which does not work here
    // use the function with rect arg instead for more consistent placement
    painter->drawText(firstLineRect, Qt::TextSingleLine, paintableString);

    painter->setFont(option.font);
    // I don't think we want to use the actual first line bounding rect because it may change depending on declenation of actual text
    QRect statusIconRect(iconAreaRect.right() + _horizontalSpacing, firstLineRect.bottom() + _lineSpacing, secondLineHeight, secondLineHeight);
    QIcon statusIcon = index.data(FolderItemRoles::StatusIconRole).value<QIcon>();
    painter->drawPixmap(statusIconRect, statusIcon.pixmap(statusIconRect.size()));
    QRect secondLineRect(statusIconRect.right() + _horizontalSpacing, firstLineRect.bottom() + _lineSpacing,
        paintableArea.width() - iconRect.width() - _horizontalSpacing - statusIconRect.width() - _cellBorder, secondLineHeight);
    // the status strings are short enough that we should not need to elide them.
    painter->drawText(secondLineRect, Qt::TextSingleLine, index.data(FolderItemRoles::StatusStringRole).toString(), nullptr);

    painter->setPen(QPen(QBrush(_separatorColor), _cellSeparatorWidth));
    painter->drawLine(option.rect.left(), option.rect.bottom(), option.rect.right(), option.rect.bottom());

    painter->restore();
}

void FolderItemDelegate::paintError(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem baseOpt(option);
    QStyleOptionButton buttonStyle;
    baseOpt.palette.setColor(QPalette::Highlight, buttonStyle.palette.color(QPalette::Highlight));
    QRect widgetRect = option.widget->rect();
    int fullWidth = widgetRect.width();
    int missingX = 0;
    const QTreeView *tree = qobject_cast<const QTreeView *>(option.widget);
    if (tree)
        missingX = tree->indentation() * 2;
    // yes this x should be negative to overtake any indent on the tree
    // opt.rect.setLeft(opt.rect.width() - fullWidth);
    baseOpt.rect.setWidth(fullWidth);
    // this allows painting over the indent area which may be incorrect color after switching dark/light mode
    // note we *can't* do this on the folder row because the expander is then invisible.
    baseOpt.rect.setLeft(-missingX);
    drawBackground(painter, baseOpt, index);

    QStyleOptionViewItem opt(option);
    QFontMetrics metrics(opt.font);
    int lineHeight = metrics.height();
    QSize iconSize(lineHeight, lineHeight);

    int indent = calculateErrorIndent(opt);

    // these placements align the error icon and text with the status icon and text of the parent folder
    opt.rect.setTop(opt.rect.top() + _cellBorder);
    opt.rect.setLeft(opt.rect.left() + indent);
    painter->drawPixmap(QRect(opt.rect.topLeft(), iconSize), index.data(Qt::DecorationRole).value<QIcon>().pixmap(iconSize));

    opt.rect.setLeft(opt.rect.left() + iconSize.width() + _horizontalSpacing);
    QRect textRect = calculateErrorTextRect(opt, index);
    painter->drawText(textRect, Qt::TextWordWrap, index.data(Qt::DisplayRole).toString());

    painter->save();
    painter->setPen(QPen(QBrush(_separatorColor), _cellSeparatorWidth));
    painter->drawLine(_cellBorder, baseOpt.rect.bottom(), widgetRect.right() - _cellBorder, baseOpt.rect.bottom());

    painter->restore();
}

QSize FolderItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // note that for the error item the default size calculated here is nuts - the width appears to be the width of the screen?
    // anyway it's really wrong. As I understand it this has something to do with the fact that the tree supports two columns
    // but we only have one "column" in the model for the error rows. I tried adding an empty item to the row but that caused other
    // issues so I'm just using brute force
    // I will try to optimize this later but for the moment, everything looks quite good in action.

    QSize defaultSize = QItemDelegate::sizeHint(option, index);
    int totalHeight = 0;

    if (index.parent().isValid()) {
        int viewWidth = option.widget->width();

        // need to compensate for the width of the button column too
        int buttonColumnWidth = 0;
        const QTreeView *tree = qobject_cast<const QTreeView *>(option.widget);
        if (tree)
            buttonColumnWidth = tree->columnWidth(1);
        int indent = calculateErrorIndent(option);
        int errorWidth = viewWidth - (indent + _cellBorder + buttonColumnWidth);

        QStyleOptionViewItem opt(option);
        opt.rect.setWidth(errorWidth);
        QRect textRect = calculateErrorTextRect(opt, index);
        totalHeight = _cellBorder * 2 + _cellSeparatorWidth + textRect.height();
        // weirdly, it does not seem to matter that I am not setting the correct width on the hint...
        // todo: see what happens if I do.
    } else {
        int firstLineHeight = 0;
        int secondLineHeight = 0;
        calculateLineHeights(option, firstLineHeight, secondLineHeight);
        totalHeight = _cellBorder * 2 + _cellSeparatorWidth + firstLineHeight + secondLineHeight + _lineSpacing;
    }

    defaultSize.setHeight(totalHeight);
    return defaultSize;
}

QRect FolderItemDelegate::calculateErrorTextRect(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString error = index.data(Qt::DisplayRole).toString();
    QRect rect = option.fontMetrics.boundingRect(option.rect, Qt::TextWordWrap, error);
    return rect;
}

int FolderItemDelegate::calculateErrorIndent(const QStyleOptionViewItem &option) const
{
    int line1 = 0;
    int line2 = 0;
    calculateLineHeights(option, line1, line2);
    int iconAreaWidth = line1 + line2 + _lineSpacing;
    // we don't want to do anything special with the tree's indent as that offset should be baked into the option rect during paint
    int totalOffset = iconAreaWidth + _horizontalSpacing;
    // possibly add the tree indent here too
    return totalOffset;
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
