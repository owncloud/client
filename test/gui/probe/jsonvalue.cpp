// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#include "jsonvalue.h"

#include "registry.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QModelIndex>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QStringList>
#include <QUrl>

namespace QtGuiProbe {

namespace {
    template <typename Size>
    QJsonObject sizeObject(const Size &size)
    {
        return {{QStringLiteral("width"), size.width()}, {QStringLiteral("height"), size.height()}};
    }

    template <typename Point>
    QJsonObject pointObject(const Point &point)
    {
        return {{QStringLiteral("x"), point.x()}, {QStringLiteral("y"), point.y()}};
    }

    template <typename Rect>
    QJsonObject rectObject(const Rect &rect)
    {
        return {{QStringLiteral("x"), rect.x()}, {QStringLiteral("y"), rect.y()}, {QStringLiteral("width"), rect.width()},
            {QStringLiteral("height"), rect.height()}};
    }

    QJsonArray listArray(const QVariantList &list, Registry &registry)
    {
        QJsonArray array;
        for (const auto &item : list) {
            array.append(toJson(item, registry));
        }
        return array;
    }
}

QJsonValue toJson(const QVariant &value, Registry &registry)
{
    if (!value.isValid() || value.metaType().id() == QMetaType::Nullptr) {
        return QJsonValue::Null;
    }
    const QMetaType type = value.metaType();
    if (type.flags() & QMetaType::PointerToQObject) {
        return registry.ref(value.value<QObject *>());
    }
    if (type.flags() & QMetaType::IsEnumeration) {
        return value.toLongLong();
    }
    switch (type.id()) {
    case QMetaType::Bool:
        return value.toBool();
    case QMetaType::Char:
    case QMetaType::SChar:
    case QMetaType::UChar:
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Long:
    case QMetaType::ULong:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        return value.toLongLong();
    case QMetaType::Float:
    case QMetaType::Double:
        return value.toDouble();
    case QMetaType::QString:
        return value.toString();
    case QMetaType::QByteArray:
        return QString::fromUtf8(value.toByteArray());
    case QMetaType::QStringList:
        return QJsonArray::fromStringList(value.toStringList());
    case QMetaType::QVariantList:
        return listArray(value.toList(), registry);
    case QMetaType::QVariantMap: {
        QJsonObject object;
        const QVariantMap map = value.toMap();
        for (auto it = map.cbegin(); it != map.cend(); ++it) {
            object.insert(it.key(), toJson(it.value(), registry));
        }
        return object;
    }
    case QMetaType::QVariantHash: {
        QJsonObject object;
        const QVariantHash hash = value.toHash();
        for (auto it = hash.cbegin(); it != hash.cend(); ++it) {
            object.insert(it.key(), toJson(it.value(), registry));
        }
        return object;
    }
    case QMetaType::QUrl:
        return value.toUrl().toString();
    case QMetaType::QDateTime:
        return value.toDateTime().toString(Qt::ISODate);
    case QMetaType::QDate:
        return value.toDate().toString(Qt::ISODate);
    case QMetaType::QTime:
        return value.toTime().toString(Qt::ISODate);
    case QMetaType::QSize:
        return sizeObject(value.toSize());
    case QMetaType::QSizeF:
        return sizeObject(value.toSizeF());
    case QMetaType::QPoint:
        return pointObject(value.toPoint());
    case QMetaType::QPointF:
        return pointObject(value.toPointF());
    case QMetaType::QRect:
        return rectObject(value.toRect());
    case QMetaType::QRectF:
        return rectObject(value.toRectF());
    case QMetaType::QModelIndex:
        return registry.ref(value.toModelIndex());
    case QMetaType::QPersistentModelIndex:
        return registry.ref(QModelIndex(value.toPersistentModelIndex()));
    case QMetaType::QJsonValue:
        return value.toJsonValue();
    case QMetaType::QJsonObject:
        return value.toJsonObject();
    case QMetaType::QJsonArray:
        return value.toJsonArray();
    default:
        break;
    }
    // other sequential containers, e.g. QList<QAction *>
    if (value.canConvert<QVariantList>()) {
        return listArray(value.toList(), registry);
    }
    if (value.canConvert<QString>()) {
        return value.toString();
    }
    return QJsonValue::Null;
}

QVariant fromJson(const QJsonValue &value, const Registry &registry)
{
    if (value.isArray()) {
        QVariantList list;
        const QJsonArray array = value.toArray();
        for (const auto &item : array) {
            list.append(fromJson(item, registry));
        }
        return list;
    }
    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        if (object.contains(QStringLiteral("$handle"))) {
            if (object.value(QStringLiteral("$handle")).isNull()) {
                return QVariant::fromValue(static_cast<QObject *>(nullptr));
            }
            const Target target = registry.resolve(object);
            if (const auto *qobject = std::get_if<QObject *>(&target)) {
                return QVariant::fromValue(*qobject);
            }
            if (const auto *index = std::get_if<QPersistentModelIndex>(&target)) {
                return QVariant::fromValue(QModelIndex(*index));
            }
            throw ProbeError{QStringLiteral("BadRequest"), QStringLiteral("a %1 cannot be passed as an argument").arg(std::get<SyntheticItem>(target).kind)};
        }
        QVariantMap map;
        for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
            map.insert(it.key(), fromJson(it.value(), registry));
        }
        return map;
    }
    return value.toVariant();
}

} // namespace QtGuiProbe
