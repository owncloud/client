// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "registry.h"

#include <QJsonObject>
#include <QJsonValue>
#include <QMetaEnum>
#include <QObjectList>
#include <QString>
#include <QVariant>

#include <functional>
#include <optional>
#include <vector>

namespace QtGuiProbe {

/// True if the selector value `want` matches `have`: {"$regex": p} is a QRegularExpression search,
/// {"$wildcard": p} a whole-string wildcard match, and a number or bool on either side compares by value
/// ("0" == 0, 1 == true, true == "true"). Everything else is a string compare.
/// Throws ProbeError (BadRequest) for an invalid pattern or an unknown $-key.
bool valueMatches(const QJsonValue &want, const QVariant &have);

/// The objects a search descends into below `object`: its QObject children, and for a QMenu its actions.
QObjectList searchChildren(const QObject *object);

/// Resolves Squish-style selectors ({"type": ..., "name": ..., "container": {...}, ...}) to targets.
class SelectorEngine
{
public:
    /// Searches `scope` for targets the widget search cannot see (model items, Qt Quick items).
    /// `scope` is the container (or window) the selector is scoped to, or a null QObject* for the whole
    /// application. Returns std::nullopt to decline; a vector claims the selector for this scope and replaces
    /// the widget search there. It must hold exactly the matches below `scope`, each once, in display order, for every key
    /// except `container`, `window` (both already applied through `scope`), `occurrence`, `aboveWidget` and
    /// `leftWidget` (the engine applies those to the merged result; with the last two, only QWidget results count).
    using TypeResolver = std::function<std::optional<std::vector<Target>>(const QJsonObject &sel, const Target &scope)>;
    /// Reads a key the QObject lookup does not know (same shape as Probe::PropertyHook).
    using PropertyReader = std::function<std::optional<QJsonValue>(const Target &target, const QString &name)>;
    /// Decides whether a target is ready (visible and enabled); std::nullopt passes to the next check.
    using ReadyCheck = std::function<std::optional<bool>(const Target &target)>;

    explicit SelectorEngine(Registry &registry);

    /// All matches in search order; `occurrence: n` reduces them to the n-th. Throws ProbeError (BadRequest)
    /// for a malformed selector, and Registry errors for a bad handle in it.
    std::vector<Target> findAll(const QJsonObject &selector);
    /// The first match (or the `occurrence`-th); with `ready`, std::nullopt unless that match is ready.
    std::optional<Target> find(const QJsonObject &selector, bool ready);
    /// True if `target` satisfies every key of `selector` except `occurrence`, `aboveWidget` and `leftWidget`.
    /// `container` and `window` are checked through the QObject ancestry, so only QObject targets pass them.
    bool matches(const Target &target, const QJsonObject &selector);
    /// Like matches(), but also ignores `container` and `window`: for resolvers that apply the scope themselves.
    bool matchesProperties(const Target &target, const QJsonObject &selector);
    /// True if `target` is visible and enabled.
    bool isReady(const Target &target) const;

    /// Resolvers are asked in registration order; the first one that does not decline wins for a scope.
    void addTypeResolver(TypeResolver resolver);
    void addPropertyReader(PropertyReader reader);
    void addReadyCheck(ReadyCheck check);

private:
    struct PropertyValue
    {
        QVariant value;
        QMetaEnum enumerator = {};
        /// Set when the value is an object, for nested selectors and handles.
        std::optional<Target> target = {};
    };

    std::vector<Target> searchScope(const QJsonObject &selector, const Target &scope);
    std::vector<Target> nearest(const QJsonObject &selector, std::vector<Target> candidates);
    bool matchesKeys(const Target &target, const QJsonObject &selector, bool checkContainer, bool checkWindow);
    bool matchesKey(const Target &target, const QString &key, const QJsonValue &want);
    bool matchesObject(const Target &target, const QJsonValue &want);
    bool matchesBuddy(QObject *object, const QJsonValue &want);
    std::optional<PropertyValue> readProperty(const Target &target, const QString &name);

    Registry &_registry;
    std::vector<TypeResolver> _resolvers;
    std::vector<PropertyReader> _propertyReaders;
    std::vector<ReadyCheck> _readyChecks;
};

} // namespace QtGuiProbe
