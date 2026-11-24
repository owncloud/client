// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "registry.h"
#include "selector.h"

#include <QJsonValue>
#include <QList>
#include <QRect>
#include <QString>

#include <optional>

class QAbstractItemView;
class QModelIndex;
class QWidget;

namespace QtGuiProbe {

/// Teaches `engine` the Squish item types:
/// - {"type": "QModelIndex", "container": <view or index>, "row", "column", "text", ...}: the direct rows of the
///   view's root index or of the container index (one level; nest `container` for deeper ones).
/// - {"type": "TabItem", "container": <QTabBar or QTabWidget>, "index", "text"}.
/// - {"type": "HeaderViewItem", "container": <QHeaderView>, "section", "text"}.
/// Also serves the item properties (itemProperty) as selector keys and adds ready checks: an item is ready when
/// its view, tab bar or header is visible and enabled.
void registerItemSupport(SelectorEngine &engine, Registry &registry);

/// The value of an item property, std::nullopt for an unknown name or a non-item target.
/// QModelIndex: text, displayText, row, column, checkState ("checked", "unchecked", "partiallyChecked", or null
/// if the item has no check state), selected, visible (shown inside the viewport).
/// TabItem: text, index, visible. HeaderViewItem: text, section (the logical index), visible.
std::optional<QJsonValue> itemProperty(const Target &target, const QString &name);

/// The item children of `target`: for an item view (not a header) the column-0 indexes below its root index,
/// for an index its column-0 child indexes. Empty for anything else.
QList<Target> itemChildren(const Target &target);

/// The view an index is shown in: the one it was last found or listed through, else a view of its model
/// (visible ones first). nullptr if there is none.
/// Limitation: the "last view" is remembered per model, process-wide, so with two views sharing one model an index
/// may resolve to the other view, and itemRect(), clicks and activateItem then go there.
/// Nothing in the suite shares a model between views.
QAbstractItemView *viewOf(const QModelIndex &index);

/// The widget whose coordinates itemRect() uses: the view's viewport for a QModelIndex, the tab bar for a
/// TabItem, the header's viewport for a HeaderViewItem, the QQuickWidget showing a Qt Quick item (quickHost), the
/// widget itself for a QWidget. nullptr otherwise.
QWidget *itemWidget(const Target &target);

/// The rectangle of `target` in itemWidget(target)'s coordinates: QAbstractItemView::visualRect for an index,
/// QTabBar::tabRect for a tab, sectionViewportPosition/sectionSize across the header's viewport for a header
/// section, quickItemRect() for a Qt Quick item, rect() for a QWidget. An invalid QRect if the item is not laid out
/// (collapsed, hidden, not in a QQuickWidget) or unknown.
QRect itemRect(const Target &target);

} // namespace QtGuiProbe
