// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#include "quick.h"

#include <QApplication>
#include <QHash>
#include <QPointer>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QRegularExpression>

namespace QtGuiProbe {

namespace {

    const QLatin1String idKey("id");
    const QLatin1String visibleKey("visible");

    // The class name suffixes of types defined in QML files (_QMLTYPE_<n>) and of objects declaring properties of
    // their own (_QML_<n>); the latter can follow the former.
    const QRegularExpression &qmlSuffix()
    {
        static const QRegularExpression expression(QStringLiteral("(?:_QML(?:TYPE)?_\\d+)+$"));
        return expression;
    }

    // The QQuickWidget rendering each offscreen window, found by looking through the widgets once.
    QHash<const QQuickWindow *, QPointer<QQuickWidget>> &hosts()
    {
        static QHash<const QQuickWindow *, QPointer<QQuickWidget>> hosts;
        return hosts;
    }

    QQuickWidget *hostOf(const QQuickWindow *window)
    {
        if (!window) {
            return nullptr;
        }
        // a cached widget may be gone, or its window's address reused
        const QPointer<QQuickWidget> cached = hosts().value(window);
        if (cached && cached->quickWindow() == window) {
            return cached;
        }
        hosts().remove(window);
        const auto widgets = QApplication::allWidgets();
        for (auto *widget : widgets) {
            auto *quickWidget = qobject_cast<QQuickWidget *>(widget);
            if (quickWidget && quickWidget->quickWindow() == window) {
                hosts().insert(window, quickWidget);
                return quickWidget;
            }
        }
        return nullptr;
    }

    const QQuickItem *quickItemOf(const Target &target)
    {
        const auto *object = std::get_if<QObject *>(&target);
        return object ? qobject_cast<const QQuickItem *>(*object) : nullptr;
    }

} // namespace

void registerQuickSupport(SelectorEngine &engine, Registry &registry)
{
    registry.registerClassNameHook([](const QObject *object) -> QString {
        if (qobject_cast<const QQuickItem *>(object) || qmlSuffix().match(QLatin1String(object->metaObject()->className())).hasMatch()) {
            return squishTypeName(object);
        }
        return {};
    });

    // below a QQuickWidget the search continues at its root item, below an item at its child items
    engine.addChildrenProvider([](const QObject *object) -> std::optional<QObjectList> {
        if (const auto *widget = qobject_cast<const QQuickWidget *>(object)) {
            QObjectList children;
            QQuickItem *root = widget->rootObject();
            if (root) {
                children.append(root);
            }
            const QObjectList objectChildren = searchChildren(object);
            for (auto *child : objectChildren) {
                if (child != root) {
                    children.append(child);
                }
            }
            return children;
        }
        if (const auto *item = qobject_cast<const QQuickItem *>(object)) {
            QObjectList children;
            const auto childItems = item->childItems();
            for (auto *child : childItems) {
                children.append(child);
            }
            return children;
        }
        return std::nullopt;
    });
    // and upwards: an item hangs from its parent item, an item at the top of the scene from its QQuickWidget
    engine.addParentProvider([](const QObject *object) -> std::optional<QObject *> {
        const auto *item = qobject_cast<const QQuickItem *>(object);
        if (!item || !item->parentItem()) {
            return std::nullopt;
        }
        QQuickItem *parent = item->parentItem();
        if (item->window() && parent == item->window()->contentItem()) {
            if (QQuickWidget *host = hostOf(item->window())) {
                return host;
            }
        }
        return parent;
    });

    engine.addPropertyOverride([](const Target &target, const QString &name) -> std::optional<QJsonValue> {
        const QQuickItem *item = quickItemOf(target);
        if (item && name == visibleKey) {
            return quickItemVisible(item);
        }
        return std::nullopt;
    });
    engine.addPropertyReader([](const Target &target, const QString &name) -> std::optional<QJsonValue> {
        const auto *object = std::get_if<QObject *>(&target);
        if (object && name == idKey && qmlContext(*object)) {
            return quickId(*object);
        }
        return std::nullopt;
    });
    engine.addReadyCheck([](const Target &target) -> std::optional<bool> {
        const QQuickItem *item = quickItemOf(target);
        if (!item) {
            return std::nullopt;
        }
        const QQuickWidget *host = quickHost(item);
        return quickItemVisible(item) && item->isEnabled() && (!host || host->isEnabled());
    });
}

QString squishTypeName(const QObject *object)
{
    QString name = QString::fromLatin1(object->metaObject()->className());
    name.remove(qmlSuffix());
    if (name.startsWith(QLatin1String("QQuick")) && name.size() > 6) {
        name.remove(0, 6);
    }
    return name;
}

QString quickId(const QObject *object)
{
    for (const QQmlContext *context = qmlContext(object); context; context = context->parentContext()) {
        const QString name = context->nameForObject(object);
        if (!name.isEmpty()) {
            return name;
        }
    }
    return {};
}

QQuickWidget *quickHost(const QQuickItem *item)
{
    return hostOf(item->window());
}

bool quickItemVisible(const QQuickItem *item)
{
    if (!item->isVisible()) {
        return false;
    }
    if (const QQuickWidget *host = quickHost(item)) {
        return host->isVisible();
    }
    return item->window() && item->window()->isVisible();
}

QRect quickItemRect(const QQuickItem *item)
{
    if (!quickHost(item)) {
        return {};
    }
    // a QQuickWidget's scene coordinates are its widget coordinates
    return item->mapRectToScene(item->boundingRect()).toAlignedRect();
}

} // namespace QtGuiProbe
