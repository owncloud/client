// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#include "actions.h"

#include "items.h"
#include "jsonvalue.h"
#include "keys.h"
#include "probe.h"
#include "quick.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QElapsedTimer>
#include <QHeaderView>
#include <QJsonArray>
#include <QMenu>
#include <QMenuBar>
#include <QMetaMethod>
#include <QMouseEvent>
#include <QPixmap>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWidget>
#include <QScreen>
#include <QSpontaneKeyEvent>
#include <QTest>
#include <QToolButton>

#include <cmath>

namespace QtGuiProbe {

namespace {

    ProbeError badRequest(const QString &message)
    {
        return {QStringLiteral("BadRequest"), message};
    }

    QObject *objectOf(const Target &target)
    {
        const auto *object = std::get_if<QObject *>(&target);
        return object ? *object : nullptr;
    }

    std::optional<int> integerOf(const QJsonValue &value)
    {
        if (!value.isDouble()) {
            return std::nullopt;
        }
        const double number = value.toDouble();
        if (std::trunc(number) != number || std::abs(number) > 1e9) {
            return std::nullopt;
        }
        return int(number);
    }

    // The integer parameter `key`, or `fallback` if it is missing or null.
    int intParam(const QJsonObject &params, const QString &key, int fallback)
    {
        const QJsonValue value = params.value(key);
        if (value.isUndefined() || value.isNull()) {
            return fallback;
        }
        const auto number = integerOf(value);
        if (!number) {
            throw badRequest(QStringLiteral("parameter '%1' must be an integer").arg(key));
        }
        return *number;
    }

    // A target for posted input, which runs later: a QObject target is checked for being alive before use.
    class LaterTarget
    {
    public:
        explicit LaterTarget(const Target &target)
            : _target(target)
            , _guard(objectOf(target))
        {
        }
        /// The target, std::nullopt if its object was destroyed meanwhile.
        std::optional<Target> get() const
        {
            if (std::holds_alternative<QObject *>(_target) && !_guard) {
                return std::nullopt;
            }
            return _target;
        }

    private:
        Target _target;
        QPointer<QObject> _guard;
    };

    // ---- where input goes

    struct ClickTarget
    {
        QPointer<QWidget> widget;
        QRect rect;
    };

    // Where `action` is shown: an open menu or menu bar, or a tool button for it.
    std::optional<ClickTarget> actionTarget(QAction *action)
    {
        const QList<QObject *> shownIn = action->associatedObjects();
        for (QObject *object : shownIn) {
            auto *widget = qobject_cast<QWidget *>(object);
            if (!widget || !widget->isVisible()) {
                continue;
            }
            QRect rect;
            if (auto *menu = qobject_cast<QMenu *>(widget)) {
                rect = menu->actionGeometry(action);
            } else if (auto *bar = qobject_cast<QMenuBar *>(widget)) {
                rect = bar->actionGeometry(action);
            } else if (auto *button = qobject_cast<QToolButton *>(widget); button && button->defaultAction() == action) {
                rect = button->rect();
            }
            if (rect.isValid()) {
                return ClickTarget{widget, rect};
            }
        }
        return std::nullopt;
    }

    // The widget the events for `target` go to and the target's rectangle in it (see itemWidget, itemRect). With
    // `scroll`, an index outside its viewport is scrolled into view first.
    std::optional<ClickTarget> clickTarget(const Target &target, bool scroll)
    {
        if (auto *action = qobject_cast<QAction *>(objectOf(target))) {
            return actionTarget(action);
        }
        if (scroll) {
            if (const auto *index = std::get_if<QPersistentModelIndex>(&target); index && index->isValid()) {
                // NOTE: viewOf() is the view the index's model was last searched or listed through (process-wide),
                // so with two views of one model the click may land in the other view. The suite has no such case.
                if (QAbstractItemView *view = viewOf(*index)) {
                    const QRect rect = view->visualRect(*index);
                    if (rect.isValid() && !view->viewport()->rect().contains(rect)) {
                        view->scrollTo(*index);
                    }
                }
            }
        }
        QWidget *widget = itemWidget(target);
        const QRect rect = itemRect(target);
        if (!widget || !rect.isValid()) {
            return std::nullopt;
        }
        return ClickTarget{widget, rect};
    }

    void sendMouse(QWidget *widget, QEvent::Type type, const QPoint &pos, Qt::MouseButton button, Qt::MouseButtons buttons, Qt::KeyboardModifiers mods)
    {
        QMouseEvent event(type, pos, widget->mapToGlobal(pos), button, buttons, mods, QPointingDevice::primaryPointingDevice());
        static QElapsedTimer clock;
        if (!clock.isValid()) {
            clock.start();
        }
        event.setTimestamp(quint64(clock.elapsed()));
        QSpontaneKeyEvent::setSpontaneous(&event);
        QApplication::instance()->notify(widget, &event);
    }

    // A click (press, release) or a double click (press, release, double click, release) as a mouse sends it.
    // Unlike QTest::mouseClick, a point (0, 0) stays the top left corner, and the double click comes with the press
    // and release before it, which QAbstractItemView needs to emit doubleClicked().
    void click(QWidget *widget, const QPoint &pos, Qt::MouseButton button, Qt::KeyboardModifiers mods, int clicks)
    {
        const QPointer<QWidget> guard(widget);
        const auto events = clicks == 2
            ? std::vector<QEvent::Type>{QEvent::MouseButtonPress, QEvent::MouseButtonRelease, QEvent::MouseButtonDblClick, QEvent::MouseButtonRelease}
            : std::vector<QEvent::Type>{QEvent::MouseButtonPress, QEvent::MouseButtonRelease};
        for (const QEvent::Type type : events) {
            // a press may open a modal dialog or a menu that deletes the widget before the release
            if (!guard) {
                return;
            }
            const Qt::MouseButtons buttons = type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::MouseButtons(button);
            sendMouse(guard, type, pos, button, buttons, mods);
        }
    }

    // Focuses `target` for typing and returns the widget its key events go to.
    QWidget *focusForTyping(const Target &target)
    {
        if (auto *item = qobject_cast<QQuickItem *>(objectOf(target))) {
            QQuickWidget *host = quickHost(item);
            if (!host) {
                return nullptr;
            }
            host->setFocus(Qt::OtherFocusReason);
            item->forceActiveFocus(Qt::OtherFocusReason);
            return host;
        }
        if (const auto *index = std::get_if<QPersistentModelIndex>(&target)) {
            QAbstractItemView *view = index->isValid() ? viewOf(*index) : nullptr;
            if (!view) {
                return nullptr;
            }
            view->setCurrentIndex(*index);
            view->setFocus(Qt::OtherFocusReason);
            return view;
        }
        auto *widget = qobject_cast<QWidget *>(objectOf(target));
        if (!widget) {
            return nullptr;
        }
        widget->setFocus(Qt::OtherFocusReason);
        // keys go where the focus went: a spin box's or an editable combo box's line edit
        while (widget->focusProxy()) {
            widget = widget->focusProxy();
        }
        return widget;
    }

    // The widget that has the keyboard: an open popup (a menu), else the focus widget, else the active window.
    QWidget *keyboardWidget()
    {
        if (QWidget *grabber = QWidget::keyboardGrabber()) {
            return grabber;
        }
        if (QWidget *popup = QApplication::activePopupWidget()) {
            return popup->focusWidget() ? popup->focusWidget() : popup;
        }
        if (QWidget *focus = QApplication::focusWidget()) {
            return focus;
        }
        return QApplication::activeWindow();
    }

    // Types `strokes` into `receiver`, or with a null `receiver` into keyboardWidget() as it is before each stroke.
    void typeKeys(QWidget *receiver, const std::vector<KeyStroke> &strokes)
    {
        const QPointer<QWidget> guard(receiver);
        const bool fixed = receiver != nullptr;
        for (const KeyStroke &stroke : strokes) {
            QWidget *widget = fixed ? guard.data() : keyboardWidget();
            // Return may have closed the dialog the widget was in
            if (!widget) {
                qWarning("qtguiprobe: no widget left to type into");
                return;
            }
            QTest::sendKeyEvent(QTest::Click, widget, stroke.key, stroke.text, stroke.mods);
        }
    }

    // ---- call

    QModelIndex indexArg(const QJsonValue &value, const Registry &registry, const QString &what)
    {
        if (value.isNull() || value.isUndefined()) {
            return {};
        }
        const QVariant variant = fromJson(value, registry);
        if (variant.metaType() == QMetaType::fromType<QModelIndex>()) {
            return variant.value<QModelIndex>();
        }
        if (variant.metaType() == QMetaType::fromType<QObject *>() && !variant.value<QObject *>()) {
            return {};
        }
        throw badRequest(QStringLiteral("%1 must be a QModelIndex").arg(what));
    }

    int intArg(const QJsonValue &value, const QString &what)
    {
        const auto number = integerOf(value);
        if (!number) {
            throw badRequest(QStringLiteral("%1 must be an integer").arg(what));
        }
        return *number;
    }

    void checkArgCount(const QJsonArray &args, qsizetype min, qsizetype max, const QString &method)
    {
        if (args.size() < min || args.size() > max) {
            throw badRequest(QStringLiteral("%1 takes %2 to %3 arguments, not %4").arg(method).arg(min).arg(max).arg(args.size()));
        }
    }

    const QStringList &modelMethods()
    {
        static const QStringList names{QStringLiteral("rowCount"), QStringLiteral("columnCount"), QStringLiteral("index"), QStringLiteral("data")};
        return names;
    }

    bool isAllowlisted(const QObject *object, const QString &name)
    {
        if (name == QLatin1String("model") && qobject_cast<const QAbstractItemView *>(object)) {
            return true;
        }
        return modelMethods().contains(name) && qobject_cast<const QAbstractItemModel *>(object);
    }

    // The allowlisted methods that are not (all) slots or Q_INVOKABLEs, called directly.
    QJsonValue callAllowlisted(QObject *object, const QString &name, const QJsonArray &args, Registry &registry)
    {
        const QString method = QStringLiteral("%1::%2").arg(registry.className(object), name);
        if (auto *view = qobject_cast<QAbstractItemView *>(object)) {
            checkArgCount(args, 0, 0, method);
            return registry.ref(view->model());
        }
        auto *model = qobject_cast<QAbstractItemModel *>(object);
        if (name == QLatin1String("rowCount") || name == QLatin1String("columnCount")) {
            checkArgCount(args, 0, 1, method);
            const QModelIndex parent = args.isEmpty() ? QModelIndex() : indexArg(args[0], registry, QStringLiteral("parent"));
            return name == QLatin1String("rowCount") ? model->rowCount(parent) : model->columnCount(parent);
        }
        if (name == QLatin1String("index")) {
            checkArgCount(args, 2, 3, method);
            const QModelIndex parent = args.size() < 3 ? QModelIndex() : indexArg(args[2], registry, QStringLiteral("parent"));
            return registry.ref(model->index(intArg(args[0], QStringLiteral("row")), intArg(args[1], QStringLiteral("column")), parent));
        }
        checkArgCount(args, 1, 2, method);
        const QModelIndex index = indexArg(args[0], registry, QStringLiteral("index"));
        const int role = args.size() < 2 ? int(Qt::DisplayRole) : intArg(args[1], QStringLiteral("role"));
        return toJson(model->data(index, role), registry);
    }

    // A JSON argument as the parameter type of a meta method.
    QVariant metaArg(const QJsonValue &value, QMetaType type, const Registry &registry, int position)
    {
        QVariant variant = fromJson(value, registry);
        if (variant.metaType() == type) {
            return variant;
        }
        if (!variant.isValid()) {
            return QVariant(type);
        }
        // a null object reference for any object pointer parameter
        if (variant.metaType() == QMetaType::fromType<QObject *>() && !variant.value<QObject *>() && (type.flags() & QMetaType::PointerToQObject)) {
            return QVariant(type);
        }
        if (!variant.convert(type)) {
            throw badRequest(QStringLiteral("argument %1 cannot be converted to %2").arg(position + 1).arg(QString::fromLatin1(type.name())));
        }
        return variant;
    }

    QJsonValue callMetaMethod(QObject *object, const QMetaMethod &method, const QJsonArray &args, Registry &registry)
    {
        constexpr int maxArgs = 10;
        if (args.size() > maxArgs) {
            throw badRequest(QStringLiteral("at most %1 arguments are supported").arg(maxArgs));
        }
        QVariant values[maxArgs];
        QGenericArgument generic[maxArgs];
        for (int i = 0; i < args.size(); ++i) {
            values[i] = metaArg(args[i], method.parameterMetaType(i), registry, i);
            generic[i] = QGenericArgument(method.parameterTypeName(i).constData(), values[i].constData());
        }
        QVariant returned;
        QGenericReturnArgument returnArgument;
        if (method.returnMetaType().isValid() && method.returnMetaType() != QMetaType::fromType<void>()) {
            returned = QVariant(method.returnMetaType());
            returnArgument = QGenericReturnArgument(method.typeName(), returned.data());
        }
        const bool invoked = method.invoke(object, Qt::DirectConnection, returnArgument, generic[0], generic[1], generic[2], generic[3], generic[4], generic[5],
            generic[6], generic[7], generic[8], generic[9]);
        if (!invoked) {
            throw badRequest(QStringLiteral("cannot invoke %1::%2").arg(registry.className(object), QString::fromLatin1(method.methodSignature())));
        }
        return returned.isValid() ? toJson(returned, registry) : QJsonValue(QJsonValue::Null);
    }

    QJsonValue call(const QJsonObject &params, Registry &registry)
    {
        const Target target = registry.resolve(params.value(QStringLiteral("handle")));
        const QString name = stringParam(params, QStringLiteral("name"));
        const QJsonValue argsValue = params.value(QStringLiteral("args"));
        if (!argsValue.isUndefined() && !argsValue.isNull() && !argsValue.isArray()) {
            throw badRequest(QStringLiteral("parameter 'args' must be an array"));
        }
        const QJsonArray args = argsValue.toArray();
        QObject *object = objectOf(target);
        if (object && isAllowlisted(object, name)) {
            return callAllowlisted(object, name, args, registry);
        }
        if (object) {
            const QByteArray key = name.toUtf8();
            const QMetaObject *metaObject = object->metaObject();
            for (int i = 0; i < metaObject->methodCount(); ++i) {
                const QMetaMethod method = metaObject->method(i);
                if ((method.methodType() == QMetaMethod::Slot || method.methodType() == QMetaMethod::Method) && method.access() == QMetaMethod::Public
                    && method.name() == key && method.parameterCount() == args.size()) {
                    return callMetaMethod(object, method, args, registry);
                }
            }
        }
        throw ProbeError{QStringLiteral("MethodNotAllowed"),
            QStringLiteral("%1::%2 with %3 arguments is not a slot, Q_INVOKABLE or allowlisted method").arg(registry.className(target), name).arg(args.size())};
    }

    // ---- input

    QJsonValue mouseClick(Probe &probe, const QJsonObject &params)
    {
        const Target target = probe.registry().resolve(params.value(QStringLiteral("handle")));
        const int clicks = intParam(params, QStringLiteral("clicks"), 1);
        if (clicks != 1 && clicks != 2) {
            throw badRequest(QStringLiteral("'clicks' must be 1 or 2"));
        }
        const int button = intParam(params, QStringLiteral("button"), int(Qt::LeftButton));
        if (button <= 0 || (button & (button - 1)) != 0 || !(Qt::MouseButtons(button) & Qt::MouseButtonMask)) {
            throw badRequest(QStringLiteral("'button' must be a single Qt::MouseButton, not %1").arg(button));
        }
        const int mods = intParam(params, QStringLiteral("modifiers"), 0);
        if (mods & ~int(Qt::KeyboardModifierMask)) {
            throw badRequest(QStringLiteral("'modifiers' must be Qt::KeyboardModifiers, not %1").arg(mods));
        }
        const bool hasX = params.contains(QStringLiteral("x")) && !params.value(QStringLiteral("x")).isNull();
        const bool hasY = params.contains(QStringLiteral("y")) && !params.value(QStringLiteral("y")).isNull();
        if (hasX != hasY) {
            throw badRequest(QStringLiteral("give both 'x' and 'y' or neither"));
        }
        std::optional<QPoint> offset;
        if (hasX) {
            offset = QPoint(intParam(params, QStringLiteral("x"), 0), intParam(params, QStringLiteral("y"), 0));
        }
        if (!clickTarget(target, false)) {
            throw badRequest(QStringLiteral("%1 is not shown, there is nowhere to click").arg(probe.registry().className(target)));
        }
        probe.postInput([later = LaterTarget(target), offset, button, mods, clicks] {
            const auto target = later.get();
            const auto where = target ? clickTarget(*target, true) : std::nullopt;
            if (!where || !where->widget) {
                qWarning("qtguiprobe: the click target went away before the click");
                return;
            }
            const QPoint pos = offset ? where->rect.topLeft() + *offset : where->rect.center();
            click(where->widget, pos, Qt::MouseButton(button), Qt::KeyboardModifiers(mods), clicks);
        });
        return QJsonValue::Null;
    }

    QJsonValue typeText(Probe &probe, const QJsonObject &params)
    {
        const QString text = stringParam(params, QStringLiteral("text"));
        const std::vector<KeyStroke> strokes = parseSquishKeys(text);
        const QJsonValue handle = params.value(QStringLiteral("handle"));
        if (handle.isUndefined() || handle.isNull()) {
            if (!keyboardWidget()) {
                throw badRequest(QStringLiteral("no widget has the keyboard focus"));
            }
            probe.postInput([strokes] { typeKeys(nullptr, strokes); });
            return QJsonValue::Null;
        }
        const Target target = probe.registry().resolve(handle);
        const QObject *object = objectOf(target);
        const bool typeable = std::holds_alternative<QPersistentModelIndex>(target) || qobject_cast<const QWidget *>(object)
            || (qobject_cast<const QQuickItem *>(object) && quickHost(qobject_cast<const QQuickItem *>(object)));
        if (!typeable) {
            throw badRequest(QStringLiteral("cannot type into a %1").arg(probe.registry().className(target)));
        }
        probe.postInput([later = LaterTarget(target), strokes] {
            const auto target = later.get();
            if (QWidget *receiver = target ? focusForTyping(*target) : nullptr) {
                typeKeys(receiver, strokes);
            } else {
                qWarning("qtguiprobe: the typing target went away before typing");
            }
        });
        return QJsonValue::Null;
    }

    QJsonValue activateItem(Probe &probe, const QJsonObject &params)
    {
        const Target target = probe.registry().resolve(params.value(QStringLiteral("handle")));
        if (auto *action = qobject_cast<QAction *>(objectOf(target))) {
            probe.postInput([action = QPointer<QAction>(action)] {
                if (!action) {
                    return;
                }
                // an action in an open menu is clicked, so the menu closes as it does for a user
                const auto shown = actionTarget(action);
                if (shown && qobject_cast<QMenu *>(shown->widget)) {
                    click(shown->widget, shown->rect.center(), Qt::LeftButton, Qt::NoModifier, 1);
                } else {
                    action->trigger();
                }
            });
            return QJsonValue::Null;
        }
        if (const auto *index = std::get_if<QPersistentModelIndex>(&target)) {
            if (!index->isValid() || !viewOf(*index)) {
                throw badRequest(QStringLiteral("the QModelIndex is not shown in a view"));
            }
            probe.postInput([index = *index] {
                // see clickTarget about which view this is
                QAbstractItemView *view = index.isValid() ? viewOf(index) : nullptr;
                if (!view) {
                    return;
                }
                view->setCurrentIndex(index);
                Q_EMIT view->activated(index);
            });
            return QJsonValue::Null;
        }
        throw badRequest(QStringLiteral("cannot activate a %1, only a QAction or a QModelIndex").arg(probe.registry().className(target)));
    }

    QJsonValue screenshot(const QJsonObject &params)
    {
        const QString path = stringParam(params, QStringLiteral("path"));
        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen) {
            throw badRequest(QStringLiteral("there is no screen"));
        }
        const QPixmap pixmap = screen->grabWindow(0);
        if (pixmap.isNull()) {
            throw badRequest(QStringLiteral("cannot grab the screen"));
        }
        if (!pixmap.save(path)) {
            throw badRequest(QStringLiteral("cannot save the screenshot to %1").arg(path));
        }
        return QJsonValue::Null;
    }

} // namespace

void registerActions(Probe &probe)
{
    Registry &registry = probe.registry();
    probe.registerMethod(QStringLiteral("call"), [&registry](const QJsonObject &params) { return call(params, registry); });
    probe.registerMethod(QStringLiteral("mouseClick"), [&probe](const QJsonObject &params) { return mouseClick(probe, params); });
    probe.registerMethod(QStringLiteral("typeText"), [&probe](const QJsonObject &params) { return typeText(probe, params); });
    probe.registerMethod(QStringLiteral("activateItem"), [&probe](const QJsonObject &params) { return activateItem(probe, params); });
    probe.registerMethod(QStringLiteral("clipboardText"), [](const QJsonObject &) -> QJsonValue { return QGuiApplication::clipboard()->text(); });
    probe.registerMethod(QStringLiteral("screenshot"), [](const QJsonObject &params) { return screenshot(params); });
    probe.registerMethod(
        QStringLiteral("activeModalWidget"), [&registry](const QJsonObject &) -> QJsonValue { return registry.ref(QApplication::activeModalWidget()); });

    probe.registerMethodNameHook([](const Target &target, const QString &name) {
        const QObject *object = objectOf(target);
        return object && isAllowlisted(object, name);
    });

    // zero-argument getters Squish reads as properties
    probe.registerPropertyHook([&registry](const Target &target, const QString &name) -> std::optional<QJsonValue> {
        QObject *object = objectOf(target);
        if (name == QLatin1String("defaultAction")) {
            if (auto *button = qobject_cast<QToolButton *>(object)) {
                return registry.ref(button->defaultAction());
            }
        } else if (name == QLatin1String("orientation")) {
            if (auto *header = qobject_cast<QHeaderView *>(object)) {
                return int(header->orientation());
            }
        }
        return std::nullopt;
    });
}

} // namespace QtGuiProbe
