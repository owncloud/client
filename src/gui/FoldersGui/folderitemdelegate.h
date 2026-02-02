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
    explicit FolderItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    const int _cellBorder = 5;
    const int _cellSeparatorWidth = 1;

    // we need a gray that works with light and dark mode so this needs to be refined
    const QColor _separatorColor = "#807F7F7F";
    void calculateLineHeights(const QStyleOptionViewItem &option, int &firstLineHeight, int &secondLineHeight) const;
};
}
