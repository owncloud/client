// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <QHash>
#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QString>

#include <functional>
#include <variant>
#include <vector>

namespace QtGuiProbe {

/// An error answered to the client as {"code": ..., "message": ..., ...extra}.
struct ProbeError
{
    QString code;
    QString message;
    /// Merged into the error object, e.g. {"isMethod": true} for NoSuchProperty.
    QJsonObject extra = {};
};

/// An item that is not a QObject of its own: a tab of a QTabBar or a section of a QHeaderView.
struct SyntheticItem
{
    QPointer<QObject> owner;
    /// The $class of the item: "TabItem" or "HeaderViewItem".
    QString kind;
    int index = -1;
};

/// A synthetic item by value (owner address, kind, index), for hashing. Holds no guard: check the owner separately.
struct SyntheticItemKey
{
    const QObject *owner = nullptr;
    QString kind;
    int index = -1;

    explicit SyntheticItemKey(const SyntheticItem &item)
        : owner(item.owner.data())
        , kind(item.kind)
        , index(item.index)
    {
    }

    friend bool operator==(const SyntheticItemKey &a, const SyntheticItemKey &b) noexcept
    {
        return a.owner == b.owner && a.index == b.index && a.kind == b.kind;
    }
    friend bool operator!=(const SyntheticItemKey &a, const SyntheticItemKey &b) noexcept { return !(a == b); }
    friend size_t qHash(const SyntheticItemKey &key, size_t seed = 0) noexcept { return qHashMulti(seed, key.owner, key.kind, key.index); }
};

/// Everything a handle can stand for.
using Target = std::variant<QObject *, QPersistentModelIndex, SyntheticItem>;

/// Maps the integer handles of the wire protocol to the objects they stand for.
class Registry
{
public:
    /// Returns the $class of an object; an empty string means "not mine".
    using ClassNameHook = std::function<QString(const QObject *object)>;

    /// {"$handle": n, "$class": ...}, or the null object for nullptr. An object keeps its handle while it lives.
    QJsonObject ref(QObject *object);
    /// A handle to a model index; the null object for an invalid index. The same index (same model, same
    /// QPersistentModelIndex, so it follows row moves) keeps its handle.
    QJsonObject ref(const QModelIndex &index);
    /// A handle to a synthetic item; the null object if its owner is gone. The same (owner, kind, index) keeps its
    /// handle.
    QJsonObject ref(const SyntheticItem &item);
    QJsonObject ref(const Target &target);

    /// Resolves {"$handle": n} (or a bare n). Throws ProbeError: BadRequest for a malformed value,
    /// NoSuchObject for an unknown or null handle, DeadObject for one whose object is gone.
    Target resolve(const QJsonValue &value) const;

    /// Drops all handles. Old handles then resolve to NoSuchObject.
    void clear();

    /// The $class of an object: the first non-empty hook result, else the QMetaObject class name.
    QString className(const QObject *object) const;
    QString className(const Target &target) const;
    /// Hooks are asked in registration order, e.g. for the QML type name of Qt Quick items.
    void registerClassNameHook(ClassNameHook hook);

    static QJsonObject nullRef();

private:
    struct Entry
    {
        Target target;
        QPointer<QObject> guard; // set for QObject targets
    };

    QJsonObject add(Target target, const QString &className, QObject *guard);

    QHash<int, Entry> _entries;
    QHash<const QObject *, int> _objectHandles;
    QHash<QPersistentModelIndex, int> _indexHandles;
    QHash<SyntheticItemKey, int> _itemHandles;
    std::vector<ClassNameHook> _classNameHooks;
    int _nextHandle = 1;
};

} // namespace QtGuiProbe
