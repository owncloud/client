// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#include "registry.h"

#include <QMetaObject>

#include <cmath>
#include <limits>

namespace QtGuiProbe {

namespace {
    const QString handleKey = QStringLiteral("$handle");
    const QString classKey = QStringLiteral("$class");

    int handleNumber(const QJsonValue &value)
    {
        const double number = value.toDouble();
        if (!value.isDouble() || std::trunc(number) != number || number < 1 || number > std::numeric_limits<int>::max()) {
            throw ProbeError{QStringLiteral("BadRequest"), QStringLiteral("a handle must be a positive integer")};
        }
        return static_cast<int>(number);
    }
}

QJsonObject Registry::nullRef()
{
    return {{handleKey, QJsonValue::Null}};
}

QJsonObject Registry::add(Target target, const QString &className, QObject *guard)
{
    const int handle = _nextHandle++;
    _entries.insert(handle, Entry{std::move(target), guard});
    return {{handleKey, handle}, {classKey, className}};
}

QJsonObject Registry::ref(QObject *object)
{
    if (!object) {
        return nullRef();
    }
    const auto known = _objectHandles.constFind(object);
    if (known != _objectHandles.cend()) {
        const auto entry = _entries.constFind(known.value());
        // a dead entry's address may have been reused by this object
        if (entry != _entries.cend() && entry->guard == object) {
            return {{handleKey, known.value()}, {classKey, className(object)}};
        }
    }
    const QJsonObject result = add(object, className(object), object);
    _objectHandles.insert(object, result.value(handleKey).toInt());
    return result;
}

QJsonObject Registry::ref(const QModelIndex &index)
{
    if (!index.isValid()) {
        return nullRef();
    }
    const QPersistentModelIndex persistent(index);
    const auto known = _indexHandles.constFind(persistent);
    if (known != _indexHandles.cend()) {
        const auto entry = _entries.constFind(known.value());
        if (entry != _entries.cend()) {
            const auto *stored = std::get_if<QPersistentModelIndex>(&entry->target);
            if (stored && stored->isValid() && *stored == persistent) {
                return {{handleKey, known.value()}, {classKey, QStringLiteral("QModelIndex")}};
            }
        }
    }
    const QJsonObject result = add(persistent, QStringLiteral("QModelIndex"), nullptr);
    _indexHandles.insert(persistent, result.value(handleKey).toInt());
    return result;
}

QJsonObject Registry::ref(const SyntheticItem &item)
{
    if (!item.owner) {
        return nullRef();
    }
    const SyntheticItemKey key(item);
    const auto known = _itemHandles.constFind(key);
    if (known != _itemHandles.cend()) {
        const auto entry = _entries.constFind(known.value());
        // a dead owner's address may have been reused
        if (entry != _entries.cend()) {
            const auto *stored = std::get_if<SyntheticItem>(&entry->target);
            if (stored && stored->owner == item.owner) {
                return {{handleKey, known.value()}, {classKey, item.kind}};
            }
        }
    }
    const QJsonObject result = add(item, item.kind, nullptr);
    _itemHandles.insert(key, result.value(handleKey).toInt());
    return result;
}

QJsonObject Registry::ref(const Target &target)
{
    if (auto *const *object = std::get_if<QObject *>(&target)) {
        return ref(*object);
    }
    if (const auto *index = std::get_if<QPersistentModelIndex>(&target)) {
        return ref(QModelIndex(*index));
    }
    return ref(std::get<SyntheticItem>(target));
}

Target Registry::resolve(const QJsonValue &value) const
{
    QJsonValue handleValue = value;
    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        if (!object.contains(handleKey)) {
            throw ProbeError{QStringLiteral("BadRequest"), QStringLiteral("expected a handle reference {\"$handle\": n}")};
        }
        handleValue = object.value(handleKey);
    }
    if (handleValue.isNull()) {
        throw ProbeError{QStringLiteral("NoSuchObject"), QStringLiteral("the null object has no properties or children")};
    }
    const int handle = handleNumber(handleValue);
    const auto entry = _entries.constFind(handle);
    if (entry == _entries.cend()) {
        throw ProbeError{QStringLiteral("NoSuchObject"), QStringLiteral("unknown handle %1").arg(handle)};
    }
    const auto dead = [handle] { return ProbeError{QStringLiteral("DeadObject"), QStringLiteral("the object of handle %1 no longer exists").arg(handle)}; };
    if (std::holds_alternative<QObject *>(entry->target)) {
        if (!entry->guard) {
            throw dead();
        }
        return entry->guard.data();
    }
    if (const auto *index = std::get_if<QPersistentModelIndex>(&entry->target)) {
        if (!index->isValid()) {
            throw dead();
        }
        return *index;
    }
    const auto &item = std::get<SyntheticItem>(entry->target);
    if (!item.owner) {
        throw dead();
    }
    return item;
}

void Registry::clear()
{
    _entries.clear();
    _objectHandles.clear();
    _indexHandles.clear();
    _itemHandles.clear();
}

QString Registry::className(const QObject *object) const
{
    for (const auto &hook : _classNameHooks) {
        const QString name = hook(object);
        if (!name.isEmpty()) {
            return name;
        }
    }
    return QString::fromLatin1(object->metaObject()->className());
}

QString Registry::className(const Target &target) const
{
    if (const auto *object = std::get_if<QObject *>(&target)) {
        return className(*object);
    }
    if (std::holds_alternative<QPersistentModelIndex>(target)) {
        return QStringLiteral("QModelIndex");
    }
    return std::get<SyntheticItem>(target).kind;
}

void Registry::registerClassNameHook(ClassNameHook hook)
{
    _classNameHooks.push_back(std::move(hook));
}

} // namespace QtGuiProbe
