// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "registry.h"
#include "selector.h"

#include <QRect>
#include <QString>

class QObject;
class QQuickItem;
class QQuickWidget;

namespace QtGuiProbe {

/// Teaches `engine` and `registry` the Qt Quick items shown in a QQuickWidget:
/// - Searches and `children` continue below a QQuickWidget at its rootObject(), and below an item at its
///   childItems(). The parent of a root object is its QQuickWidget, so `container` and `window` work upwards too.
/// - The $class (and `type`) of an item is squishTypeName().
/// - `id` is the item's QML id (empty if it has none).
/// - `visible` is the item's own visibility combined with its QQuickWidget's.
/// - An item is ready when it is visible (as above) and enabled, and so is its QQuickWidget.
void registerQuickSupport(SelectorEngine &engine, Registry &registry);

/// The Squish type name of a Qt Quick object: the meta-object class name without a "_QMLTYPE_<n>" or "_QML_<n>"
/// suffix and without a leading "QQuick". QQuickTextField gives "TextField", SpaceDelegate_QMLTYPE_3 "SpaceDelegate".
QString squishTypeName(const QObject *object);

/// The QML id of `object`, looked up in its QML context and the contexts above it; empty if it has none.
QString quickId(const QObject *object);

/// The QQuickWidget showing `item`, nullptr if it is not shown in one.
QQuickWidget *quickHost(const QQuickItem *item);

/// True if `item` and the QQuickWidget showing it are visible; for an item not shown in a QQuickWidget, if it and its
/// window are visible.
bool quickItemVisible(const QQuickItem *item);

/// The rectangle of `item` in the coordinates of quickHost(item): mapRectToScene(boundingRect()).
/// An invalid QRect if the item is not shown in a QQuickWidget.
QRect quickItemRect(const QQuickItem *item);

} // namespace QtGuiProbe
