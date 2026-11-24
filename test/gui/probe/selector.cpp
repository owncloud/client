// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#include "selector.h"

#include "jsonvalue.h"

#include <QAction>
#include <QApplication>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QLabel>
#include <QMenu>
#include <QMetaEnum>
#include <QMetaProperty>
#include <QRegularExpression>
#include <QSet>
#include <QWidget>
#include <QWindow>

#include <algorithm>
#include <cmath>
#include <limits>

namespace QtGuiProbe {

namespace {

    const QLatin1String typeKey("type");
    const QLatin1String nameKey("name");
    const QLatin1String unnamedKey("unnamed");
    const QLatin1String occurrenceKey("occurrence");
    const QLatin1String containerKey("container");
    const QLatin1String windowKey("window");
    const QLatin1String aboveWidgetKey("aboveWidget");
    const QLatin1String leftWidgetKey("leftWidget");
    const QLatin1String buddyKey("buddy");
    const QLatin1String handleKey("$handle");
    const QLatin1String regexKey("$regex");
    const QLatin1String wildcardKey("$wildcard");

    ProbeError badRequest(const QString &message)
    {
        return {QStringLiteral("BadRequest"), message};
    }

    bool hasDollarKey(const QJsonObject &object)
    {
        for (auto it = object.begin(); it != object.end(); ++it) {
            if (it.key().startsWith(QLatin1Char('$'))) {
                return true;
            }
        }
        return false;
    }

    bool isHandle(const QJsonValue &value)
    {
        return value.isObject() && value.toObject().contains(handleKey);
    }

    // The compiled pattern of {"$regex": ...} or {"$wildcard": ...}; throws BadRequest for anything else with a $-key.
    QRegularExpression patternOf(const QJsonObject &want)
    {
        if (want.size() == 1 && want.value(regexKey).isString()) {
            QRegularExpression expression(want.value(regexKey).toString());
            if (!expression.isValid()) {
                throw badRequest(QStringLiteral("invalid $regex %1: %2").arg(want.value(regexKey).toString(), expression.errorString()));
            }
            return expression;
        }
        if (want.size() == 1 && want.value(wildcardKey).isString()) {
            QRegularExpression expression(
                QRegularExpression::wildcardToRegularExpression(want.value(wildcardKey).toString(), QRegularExpression::NonPathWildcardConversion));
            if (!expression.isValid()) {
                throw badRequest(QStringLiteral("invalid $wildcard %1").arg(want.value(wildcardKey).toString()));
            }
            return expression;
        }
        throw badRequest(QStringLiteral("unsupported selector value %1").arg(QString::fromUtf8(QJsonDocument(want).toJson(QJsonDocument::Compact))));
    }

    std::optional<double> numberOfString(const QString &text)
    {
        const QString trimmed = text.trimmed();
        if (trimmed.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0) {
            return 1;
        }
        if (trimmed.compare(QLatin1String("false"), Qt::CaseInsensitive) == 0) {
            return 0;
        }
        bool ok = false;
        const double number = trimmed.toDouble(&ok);
        if (ok && std::isfinite(number)) {
            return number;
        }
        return std::nullopt;
    }

    std::optional<double> numberOfJson(const QJsonValue &value)
    {
        switch (value.type()) {
        case QJsonValue::Bool:
            return value.toBool() ? 1 : 0;
        case QJsonValue::Double:
            return value.toDouble();
        case QJsonValue::String:
            return numberOfString(value.toString());
        default:
            return std::nullopt;
        }
    }

    bool isStringVariant(const QVariant &value)
    {
        const int id = value.metaType().id();
        return id == QMetaType::QString || id == QMetaType::QByteArray || id == QMetaType::QChar;
    }

    std::optional<double> numberOfVariant(const QVariant &value)
    {
        switch (value.metaType().id()) {
        case QMetaType::Bool:
            return value.toBool() ? 1 : 0;
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::Long:
        case QMetaType::ULong:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::Short:
        case QMetaType::UShort:
        case QMetaType::Char:
        case QMetaType::SChar:
        case QMetaType::UChar:
        case QMetaType::Float:
        case QMetaType::Double:
            return value.toDouble();
        case QMetaType::QString:
        case QMetaType::QByteArray:
            return numberOfString(value.toString());
        default:
            break;
        }
        if (value.metaType().flags() & QMetaType::IsEnumeration) {
            bool ok = false;
            const qlonglong number = value.toLongLong(&ok);
            if (ok) {
                return static_cast<double>(number);
            }
        }
        return std::nullopt;
    }

    QString textOfJson(const QJsonValue &value)
    {
        switch (value.type()) {
        case QJsonValue::Bool:
            return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
        case QJsonValue::Double: {
            const double number = value.toDouble();
            if (std::trunc(number) == number && std::abs(number) < 1e15) {
                return QString::number(static_cast<qint64>(number));
            }
            return QString::number(number);
        }
        default:
            return value.toString();
        }
    }

    // Checks the $-values of a selector before any object is compared, so a bad one fails even without candidates.
    void validate(const QJsonObject &selector)
    {
        for (auto it = selector.begin(); it != selector.end(); ++it) {
            if (!it->isObject()) {
                continue;
            }
            const QJsonObject value = it->toObject();
            if (value.contains(handleKey)) {
                continue;
            }
            if (hasDollarKey(value)) {
                patternOf(value);
            } else {
                validate(value);
            }
        }
    }

    int occurrenceOf(const QJsonObject &selector)
    {
        const QJsonValue value = selector.value(occurrenceKey);
        if (value.isUndefined()) {
            return 0;
        }
        const std::optional<double> number = value.isBool() ? std::nullopt : numberOfJson(value);
        if (!number || *number < 1 || std::trunc(*number) != *number || *number > std::numeric_limits<int>::max()) {
            throw badRequest(QStringLiteral("occurrence must be a positive integer"));
        }
        return static_cast<int>(*number);
    }

    bool sameTarget(const Target &a, const Target &b)
    {
        if (a.index() != b.index()) {
            return false;
        }
        if (const auto *object = std::get_if<QObject *>(&a)) {
            return *object == std::get<QObject *>(b);
        }
        if (const auto *index = std::get_if<QPersistentModelIndex>(&a)) {
            return *index == std::get<QPersistentModelIndex>(b);
        }
        const auto &x = std::get<SyntheticItem>(a);
        const auto &y = std::get<SyntheticItem>(b);
        return x.owner == y.owner && x.kind == y.kind && x.index == y.index;
    }

    // Appends targets to a result, skipping ones already in it; hashed, as model scopes can hold many items.
    class Merger
    {
    public:
        void merge(std::vector<Target> &result, std::vector<Target> targets)
        {
            for (auto &target : targets) {
                if (insert(target)) {
                    result.push_back(std::move(target));
                }
            }
        }

    private:
        bool insert(const Target &target)
        {
            if (const auto *object = std::get_if<QObject *>(&target)) {
                return insertInto(_objects, *object);
            }
            if (const auto *index = std::get_if<QPersistentModelIndex>(&target)) {
                return insertInto(_indexes, *index);
            }
            return insertInto(_items, SyntheticItemKey(std::get<SyntheticItem>(target)));
        }

        template <typename T>
        static bool insertInto(QSet<T> &set, const T &value)
        {
            const auto size = set.size();
            set.insert(value);
            return set.size() != size;
        }

        QSet<QObject *> _objects;
        QSet<QPersistentModelIndex> _indexes;
        QSet<SyntheticItemKey> _items;
    };

    // The objects a QObject hangs from: its parent and, for an action, the widgets showing it (e.g. a menu).
    QObjectList logicalParents(const SelectorEngine &engine, const QObject *object)
    {
        QObjectList parents;
        if (QObject *parent = engine.parentOf(object)) {
            parents.append(parent);
        }
        if (const auto *action = qobject_cast<const QAction *>(object)) {
            const auto associated = action->associatedObjects();
            for (auto *widget : associated) {
                if (!parents.contains(widget)) {
                    parents.append(widget);
                }
            }
        }
        return parents;
    }

    QObjectList logicalAncestors(const SelectorEngine &engine, QObject *object)
    {
        QObjectList ancestors;
        QObjectList pending = logicalParents(engine, object);
        while (!pending.isEmpty()) {
            QObject *next = pending.takeFirst();
            if (next == object || ancestors.contains(next)) {
                continue;
            }
            ancestors.append(next);
            pending.append(logicalParents(engine, next));
        }
        return ancestors;
    }

    // A widget Squish names other objects by: a window, or a dialog or main window embedded in another window
    // (reparented into a layout with its flags reset to Qt::Widget, so window() passes over it).
    bool isWindowLike(const QWidget *widget)
    {
        return widget->isWindow() || widget->inherits("QDialog") || widget->inherits("QMainWindow");
    }

    // The windows an object counts as belonging to, innermost first: every window-like widget from the object itself
    // up to its window(), and for a popup (a menu) also those it pops up from, as Squish names a context menu by its
    // parent's window. A dialog is not chained to the window it is parented to. A non-widget (an action, a Qt Quick
    // item) belongs to its parents' windows.
    QObjectList windowsOf(const SelectorEngine &engine, QObject *object)
    {
        QObjectList windows;
        QObjectList pending{object};
        QSet<QObject *> visited;
        while (!pending.isEmpty()) {
            QObject *next = pending.takeFirst();
            if (visited.contains(next)) {
                continue;
            }
            visited.insert(next);
            if (auto *widget = qobject_cast<QWidget *>(next)) {
                while (widget) {
                    if (isWindowLike(widget) && !windows.contains(widget)) {
                        windows.append(widget);
                    }
                    if (widget->isWindow() && widget->windowType() != Qt::Popup) {
                        break;
                    }
                    widget = widget->parentWidget();
                }
            } else {
                pending.append(logicalParents(engine, next));
            }
        }
        return windows;
    }

    // Parentless top-level widgets, the ones shown first first (QGuiApplication keeps its windows in creation order,
    // and a widget creates its window when it is first shown); never shown ones last.
    QObjectList topLevelRoots()
    {
        const QWindowList windows = QGuiApplication::topLevelWindows();
        std::vector<std::pair<qsizetype, QWidget *>> ranked;
        const auto widgets = QApplication::topLevelWidgets();
        for (auto *widget : widgets) {
            if (widget->parentWidget()) {
                continue;
            }
            const qsizetype rank = widget->windowHandle() ? windows.indexOf(widget->windowHandle()) : -1;
            ranked.emplace_back(rank < 0 ? std::numeric_limits<qsizetype>::max() : rank, widget);
        }
        std::stable_sort(ranked.begin(), ranked.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
        QObjectList roots;
        for (const auto &entry : ranked) {
            roots.append(entry.second);
        }
        return roots;
    }

    QRect globalRect(const QWidget *widget)
    {
        return QRect(widget->mapToGlobal(QPoint(0, 0)), widget->size());
    }

} // namespace

bool valueMatches(const QJsonValue &want, const QVariant &have)
{
    if (want.isObject()) {
        const QJsonObject object = want.toObject();
        if (!hasDollarKey(object)) {
            return false; // a nested selector needs an object to match against
        }
        return patternOf(object).match(have.toString()).hasMatch();
    }
    if (want.isNull() || want.isUndefined()) {
        return !have.isValid() || have.isNull();
    }
    if (want.isArray()) {
        return false;
    }
    // two strings compare as text; with a number or bool on either side, by value
    if (!(want.isString() && isStringVariant(have))) {
        const std::optional<double> wanted = numberOfJson(want);
        const std::optional<double> had = numberOfVariant(have);
        if (wanted && had) {
            return *wanted == *had;
        }
    }
    return textOfJson(want) == have.toString();
}

QObjectList searchChildren(const QObject *object)
{
    QObjectList children = object->children();
    // a menu's actions need not be its children
    if (const auto *menu = qobject_cast<const QMenu *>(object)) {
        const auto actions = menu->actions();
        for (auto *action : actions) {
            if (action->parent() != menu) {
                children.append(action);
            }
        }
    }
    return children;
}

SelectorEngine::SelectorEngine(Registry &registry)
    : _registry(registry)
{
}

void SelectorEngine::addTypeResolver(TypeResolver resolver)
{
    _resolvers.push_back(std::move(resolver));
}

void SelectorEngine::addPropertyReader(PropertyReader reader)
{
    _propertyReaders.push_back(std::move(reader));
}

void SelectorEngine::addPropertyOverride(PropertyReader reader)
{
    _propertyOverrides.push_back(std::move(reader));
}

void SelectorEngine::addReadyCheck(ReadyCheck check)
{
    _readyChecks.push_back(std::move(check));
}

void SelectorEngine::addChildrenProvider(ChildrenProvider provider)
{
    _childrenProviders.push_back(std::move(provider));
}

void SelectorEngine::addParentProvider(ParentProvider provider)
{
    _parentProviders.push_back(std::move(provider));
}

QObjectList SelectorEngine::childrenOf(const QObject *object) const
{
    for (const auto &provider : _childrenProviders) {
        if (auto children = provider(object)) {
            return std::move(*children);
        }
    }
    return searchChildren(object);
}

QObject *SelectorEngine::parentOf(const QObject *object) const
{
    for (const auto &provider : _parentProviders) {
        if (const auto parent = provider(object)) {
            return *parent;
        }
    }
    return object->parent();
}

std::vector<Target> SelectorEngine::findAll(const QJsonObject &selector)
{
    if (selector.contains(handleKey)) {
        return {_registry.resolve(selector)};
    }
    validate(selector);
    const int occurrence = occurrenceOf(selector);

    // `container` (else `window`) narrows the search to the subtrees of the objects it matches
    std::vector<Target> scopeTargets;
    const QLatin1String scopeKey = selector.contains(containerKey) ? containerKey : windowKey;
    const QJsonValue scope = selector.value(scopeKey);
    if (scope.isUndefined()) {
        scopeTargets.push_back(static_cast<QObject *>(nullptr));
    } else if (!scope.isObject()) {
        throw badRequest(QStringLiteral("%1 must be a selector or a handle").arg(scopeKey));
    } else {
        scopeTargets = findAll(scope.toObject());
    }

    std::vector<Target> result;
    if (scopeTargets.size() == 1) {
        // one search yields each target once
        result = searchScope(selector, scopeTargets.front());
    } else {
        Merger merger;
        for (const auto &scopeTarget : scopeTargets) {
            merger.merge(result, searchScope(selector, scopeTarget));
        }
    }

    if (selector.contains(aboveWidgetKey) || selector.contains(leftWidgetKey)) {
        result = nearest(selector, std::move(result));
    }
    if (occurrence > 0) {
        if (static_cast<size_t>(occurrence) > result.size()) {
            return {};
        }
        return {result[occurrence - 1]};
    }
    return result;
}

std::vector<Target> SelectorEngine::searchScope(const QJsonObject &selector, const Target &scope)
{
    for (const auto &resolver : _resolvers) {
        if (auto claimed = resolver(selector, scope)) {
            return std::move(*claimed);
        }
    }
    const auto *scopeObject = std::get_if<QObject *>(&scope);
    if (!scopeObject) {
        return {};
    }
    std::vector<Target> found;
    QSet<QObject *> visited;
    // depth first, in children order; the scope itself is not a candidate
    std::vector<QObject *> pending;
    const QObjectList roots = *scopeObject ? childrenOf(*scopeObject) : topLevelRoots();
    pending.assign(roots.rbegin(), roots.rend());
    if (*scopeObject) {
        visited.insert(*scopeObject);
    }
    while (!pending.empty()) {
        QObject *object = pending.back();
        pending.pop_back();
        if (visited.contains(object)) {
            continue;
        }
        visited.insert(object);
        if (matchesKeys(object, selector, false, true)) {
            found.push_back(object);
        }
        const QObjectList children = childrenOf(object);
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            pending.push_back(*it);
        }
    }
    return found;
}

std::vector<Target> SelectorEngine::nearest(const QJsonObject &selector, std::vector<Target> candidates)
{
    // the widget named by aboveWidget is above the one searched for, the one named by leftWidget left of it
    struct Anchor
    {
        QRect rect;
        bool above;
    };
    std::vector<Anchor> anchors;
    for (const auto &key : {aboveWidgetKey, leftWidgetKey}) {
        const QJsonValue value = selector.value(key);
        if (value.isUndefined()) {
            continue;
        }
        if (!value.isObject()) {
            throw badRequest(QStringLiteral("%1 must be a selector or a handle").arg(key));
        }
        const std::optional<Target> anchor = find(value.toObject(), false);
        const auto *object = anchor ? std::get_if<QObject *>(&*anchor) : nullptr;
        const auto *widget = object ? qobject_cast<QWidget *>(*object) : nullptr;
        if (!widget) {
            return {};
        }
        anchors.push_back({globalRect(widget), key == aboveWidgetKey});
    }

    std::optional<Target> best;
    qint64 bestDistance = std::numeric_limits<qint64>::max();
    for (const auto &candidate : candidates) {
        const auto *object = std::get_if<QObject *>(&candidate);
        const auto *widget = object ? qobject_cast<QWidget *>(*object) : nullptr;
        if (!widget) {
            continue;
        }
        const QRect rect = globalRect(widget);
        qint64 distance = 0;
        bool fits = true;
        for (const auto &anchor : anchors) {
            const QRect &a = anchor.rect;
            if (anchor.above) {
                const bool overlaps = rect.x() < a.x() + a.width() && a.x() < rect.x() + rect.width();
                fits = fits && overlaps && a.y() + a.height() <= rect.y();
                distance += rect.y() - (a.y() + a.height());
            } else {
                const bool overlaps = rect.y() < a.y() + a.height() && a.y() < rect.y() + rect.height();
                fits = fits && overlaps && a.x() + a.width() <= rect.x();
                distance += rect.x() - (a.x() + a.width());
            }
        }
        if (fits && distance < bestDistance) {
            best = candidate;
            bestDistance = distance;
        }
    }
    if (!best) {
        return {};
    }
    return {*best};
}

std::optional<Target> SelectorEngine::find(const QJsonObject &selector, bool ready)
{
    const std::vector<Target> found = findAll(selector);
    if (found.empty()) {
        return std::nullopt;
    }
    if (ready && !isReady(found.front())) {
        return std::nullopt;
    }
    return found.front();
}

bool SelectorEngine::matches(const Target &target, const QJsonObject &selector)
{
    validate(selector);
    return matchesKeys(target, selector, true, true);
}

bool SelectorEngine::matchesProperties(const Target &target, const QJsonObject &selector)
{
    validate(selector);
    return matchesKeys(target, selector, false, false);
}

bool SelectorEngine::isReady(const Target &target) const
{
    for (const auto &check : _readyChecks) {
        if (const auto ready = check(target)) {
            return *ready;
        }
    }
    if (const auto *object = std::get_if<QObject *>(&target)) {
        if (const auto *widget = qobject_cast<const QWidget *>(*object)) {
            return widget->isVisible() && widget->isEnabled();
        }
        // QAction, Qt Quick items and the like: their visible and enabled properties, where they have them
        const QMetaObject *metaObject = (*object)->metaObject();
        for (const char *name : {"visible", "enabled"}) {
            const int index = metaObject->indexOfProperty(name);
            if (index >= 0 && !metaObject->property(index).read(*object).toBool()) {
                return false;
            }
        }
        return true;
    }
    if (const auto *item = std::get_if<SyntheticItem>(&target)) {
        return item->owner && isReady(item->owner.data());
    }
    return std::get<QPersistentModelIndex>(target).isValid();
}

bool SelectorEngine::matchesKeys(const Target &target, const QJsonObject &selector, bool checkContainer, bool checkWindow)
{
    // cheap keys first: they rule out most candidates
    for (const auto &key : {typeKey, nameKey, unnamedKey}) {
        const QJsonValue want = selector.value(key);
        if (!want.isUndefined() && !matchesKey(target, key, want)) {
            return false;
        }
    }
    for (auto it = selector.begin(); it != selector.end(); ++it) {
        const QString key = it.key();
        if (key == typeKey || key == nameKey || key == unnamedKey || key == occurrenceKey || key == aboveWidgetKey || key == leftWidgetKey
            || key == containerKey || key == windowKey) {
            continue;
        }
        if (!matchesKey(target, key, it.value())) {
            return false;
        }
    }
    const auto *object = std::get_if<QObject *>(&target);
    if (checkWindow && selector.contains(windowKey)) {
        if (!object) {
            return false;
        }
        const QObjectList windows = windowsOf(*this, *object);
        if (!std::any_of(windows.begin(), windows.end(), [&](QObject *window) { return matchesObject(window, selector.value(windowKey)); })) {
            return false;
        }
    }
    if (checkContainer && selector.contains(containerKey)) {
        if (!object) {
            return false;
        }
        const QObjectList ancestors = logicalAncestors(*this, *object);
        if (!std::any_of(ancestors.begin(), ancestors.end(), [&](QObject *ancestor) { return matchesObject(ancestor, selector.value(containerKey)); })) {
            return false;
        }
    }
    return true;
}

bool SelectorEngine::matchesObject(const Target &target, const QJsonValue &want)
{
    if (!want.isObject()) {
        throw badRequest(QStringLiteral("expected a selector or a handle"));
    }
    if (isHandle(want)) {
        return sameTarget(target, _registry.resolve(want));
    }
    return matchesKeys(target, want.toObject(), true, true);
}

bool SelectorEngine::matchesKey(const Target &target, const QString &key, const QJsonValue &want)
{
    const auto *object = std::get_if<QObject *>(&target);
    if (key == typeKey) {
        return valueMatches(want, _registry.className(target));
    }
    if (object && key == nameKey) {
        return valueMatches(want, (*object)->objectName());
    }
    if (key == unnamedKey) {
        return valueMatches(want, object ? (*object)->objectName().isEmpty() : true);
    }

    const std::optional<PropertyValue> property = readProperty(target, key);
    if (!property) {
        // Squish's "buddy" names the label whose buddy this widget is
        return object && key == buddyKey && matchesBuddy(*object, want);
    }
    if (want.isObject() && (isHandle(want) || !hasDollarKey(want.toObject()))) {
        return property->target && matchesObject(*property->target, want);
    }
    if (property->enumerator.isValid() && want.isString() && !numberOfString(want.toString())) {
        const QString wanted = want.toString().section(QStringLiteral("::"), -1);
        bool ok = false;
        const int value = property->value.toInt(&ok);
        if (ok) {
            const QByteArray keys =
                property->enumerator.isFlag() ? property->enumerator.valueToKeys(value) : QByteArray(property->enumerator.valueToKey(value));
            return QString::fromLatin1(keys) == wanted;
        }
    }
    return valueMatches(want, property->value);
}

bool SelectorEngine::matchesBuddy(QObject *object, const QJsonValue &want)
{
    auto *widget = qobject_cast<QWidget *>(object);
    if (!widget || !want.isObject()) {
        return false;
    }
    const auto labels = widget->window()->findChildren<QLabel *>();
    return std::any_of(
        labels.begin(), labels.end(), [&](QLabel *label) { return label->buddy() == widget && matchesObject(static_cast<QObject *>(label), want); });
}

std::optional<QJsonValue> SelectorEngine::ask(const std::vector<PropertyReader> &readers, const Target &target, const QString &name)
{
    for (const auto &reader : readers) {
        if (auto value = reader(target, name)) {
            return value;
        }
    }
    return std::nullopt;
}

std::optional<QJsonValue> SelectorEngine::readValue(const Target &target, const QString &name)
{
    if (auto value = ask(_propertyOverrides, target, name)) {
        return value;
    }
    if (const auto *object = std::get_if<QObject *>(&target)) {
        const QByteArray key = name.toUtf8();
        const QMetaObject *metaObject = (*object)->metaObject();
        const int index = metaObject->indexOfProperty(key.constData());
        if (index >= 0) {
            return toJson(metaObject->property(index).read(*object), _registry);
        }
        if ((*object)->dynamicPropertyNames().contains(key)) {
            return toJson((*object)->property(key.constData()), _registry);
        }
    }
    return ask(_propertyReaders, target, name);
}

std::optional<SelectorEngine::PropertyValue> SelectorEngine::fromReader(const QJsonValue &value)
{
    if (isHandle(value)) {
        if (value.toObject().value(handleKey).isNull()) {
            return PropertyValue{};
        }
        return PropertyValue{QVariant(), QMetaEnum(), _registry.resolve(value)};
    }
    return PropertyValue{value.toVariant()};
}

// The same order as readValue(), keeping the enumerator and the object of a QObject's own property.
std::optional<SelectorEngine::PropertyValue> SelectorEngine::readProperty(const Target &target, const QString &name)
{
    if (const auto value = ask(_propertyOverrides, target, name)) {
        return fromReader(*value);
    }
    if (const auto *object = std::get_if<QObject *>(&target)) {
        const QByteArray key = name.toUtf8();
        const QMetaObject *metaObject = (*object)->metaObject();
        const int index = metaObject->indexOfProperty(key.constData());
        std::optional<PropertyValue> result;
        if (index >= 0) {
            const QMetaProperty property = metaObject->property(index);
            result = PropertyValue{property.read(*object), property.isEnumType() ? property.enumerator() : QMetaEnum()};
        } else if ((*object)->dynamicPropertyNames().contains(key)) {
            result = PropertyValue{(*object)->property(key.constData())};
        }
        if (result) {
            if (result->value.metaType().flags() & QMetaType::PointerToQObject) {
                if (auto *pointee = result->value.value<QObject *>()) {
                    result->target = pointee;
                }
            }
            return result;
        }
    }
    if (const auto value = ask(_propertyReaders, target, name)) {
        return fromReader(*value);
    }
    return std::nullopt;
}

} // namespace QtGuiProbe
