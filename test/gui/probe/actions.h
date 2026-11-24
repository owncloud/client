// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#pragma once

namespace QtGuiProbe {

class Probe;

/// Adds the methods that call into and drive the application:
/// - `call` {handle, name, args}: a slot or Q_INVOKABLE of that name taking args.size() arguments, or an allowlisted
///   method: QAbstractItemView::model(), QAbstractItemModel::rowCount([parent]), columnCount([parent]),
///   index(row, column[, parent]) and data(index[, role]). Anything else is MethodNotAllowed. Runs synchronously.
/// - `mouseClick` {handle, x?, y?, button, modifiers, clicks: 1|2}: at the item's rect topLeft() + (x, y), or its
///   centre, in itemWidget()'s coordinates; a QAction is clicked where its menu or tool button shows it.
/// - `clickButton` {handle}: a left click, as Squish's clickButton. A QAbstractButton is clicked at a point its
///   hitButton() accepts: the centre, else the centre of the style's indicator or contents rect (a radio button or
///   check box stretched by its layout ignores presses beside its indicator and text); BadRequest naming the button
///   if it is hidden or accepts none of them. Any other target is clicked as `mouseClick` without x and y does.
/// - `typeText` {handle?, text}: Squish key syntax (parseSquishKeys) into the focused target, or without a handle
///   into the widget that has the keyboard (popup, focus widget, active window).
/// - `activateItem` {handle}: QAction::trigger(), or a click if the action is shown in an open menu; for a
///   QModelIndex setCurrentIndex() plus the view's activated() signal.
/// - `clipboardText`, `screenshot` {path} (the primary screen), `activeModalWidget`.
/// Input (mouseClick, clickButton, typeText, activateItem) is validated, then posted with Probe::postInput() and
/// answered at once, so input that opens a modal dialog does not block the protocol.
/// Also serves the zero-argument getters QToolButton::defaultAction and QHeaderView::orientation as properties, and
/// reports the allowlisted methods through `isMethod`.
void registerActions(Probe &probe);

} // namespace QtGuiProbe
