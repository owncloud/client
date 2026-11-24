// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QJsonValue>
#include <QVariant>

namespace QtGuiProbe {

class Registry;

/// Converts a property or return value to JSON. QObject pointers become handle references, QDateTime/QDate/QTime
/// become ISO 8601 strings, QUrl a string, QSize/QRect/QPoint (and their F variants) objects, sequences arrays.
/// Values that have no JSON form and no string conversion become null.
QJsonValue toJson(const QVariant &value, Registry &registry);

/// Converts a JSON argument to a QVariant. Handle references resolve to the QObject* or QModelIndex they stand
/// for (throws ProbeError like Registry::resolve); everything else follows QJsonValue::toVariant().
QVariant fromJson(const QJsonValue &value, const Registry &registry);

} // namespace QtGuiProbe
