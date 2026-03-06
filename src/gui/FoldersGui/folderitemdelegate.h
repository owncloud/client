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

#pragma once

#include <QColor>
#include <QItemDelegate>

namespace OCC {


class FolderItemDelegate : public QItemDelegate
{
public:
    explicit FolderItemDelegate(int treeIndentation, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    const int _cellBorder = 10;
    const int _horizontalSpacing = 10;
    const int _cellSeparatorWidth = 1;
    const int _namePixelSize = 14;
    const int _lineSpacing = 5;
    int _treeIndentation;

    // we need a gray that works with light and dark mode so this needs to be refined
    const QColor _separatorColor = "#807F7F7F";
    void calculateLineHeights(const QStyleOptionViewItem &option, int &firstLineHeight, int &secondLineHeight) const;
    void paintError(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    int calculateErrorIndent(const QStyleOptionViewItem &option) const;
    /**
     * @brief calculateErrorTextRect
     * @param option for the current cell, wherin the rect has been set to the *actual* available width for the painted
     * text as it will appear in the cell. Option height is irrelevant for the calculation so may be zero
     * @param index of the item
     * @return the bounding rectangle of the error text for the item with the option rectangle's given width, and the height is calculated
     * based on the number of lines created when word wrap is applied (if necessary)
     */
    QRect calculateErrorTextRect(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    /**
     * @brief errorSizeHint is a specialized size hint for error rows in the tree
     * @param option the usual
     * @param index
     * @return
     */
    QSize errorSizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};
}
