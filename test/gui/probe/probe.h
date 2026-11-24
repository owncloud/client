// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "registry.h"
#include "selector.h"

#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>

#include <functional>
#include <optional>
#include <vector>

class QLocalServer;
class QLocalSocket;

namespace QtGuiProbe {

/// Serves the line-based JSON protocol on a QLocalServer, one connection at a time, on the GUI thread.
class Probe : public QObject
{
    Q_OBJECT
public:
    /// A protocol method. Throw ProbeError to answer with an error.
    using Method = std::function<QJsonValue(const QJsonObject &params)>;
    /// Serves `get` for a name the built-in lookup does not know (zero-argument getters, item properties).
    /// Returns std::nullopt to pass.
    using PropertyHook = std::function<std::optional<QJsonValue>(const Target &target, const QString &name)>;
    /// Reports whether `name` is a callable method of `target` (the "isMethod" of a NoSuchProperty error).
    using MethodNameHook = std::function<bool(const Target &target, const QString &name)>;
    /// Appends further children of `target` (e.g. model items) to `children`.
    using ChildrenHook = std::function<void(const Target &target, QList<Target> &children)>;

    explicit Probe(QObject *parent = nullptr);
    ~Probe() override;

    /// Starts the probe on the socket named by QT_GUI_TEST_PROBE; nullptr if it is unset or listening fails.
    /// A second call returns the running probe.
    static Probe *startIfRequested();

    /// Listens on `name` (a full path on Unix), replacing a stale socket of that name.
    bool listen(const QString &name);

    /// Answers one request object; the socket handler calls this for every line.
    QJsonObject handle(const QJsonObject &request);

    /// Adds or replaces a protocol method.
    void registerMethod(const QString &name, Method method);
    void registerPropertyHook(PropertyHook hook);
    void registerMethodNameHook(MethodNameHook hook);
    void registerChildrenHook(ChildrenHook hook);

    Registry &registry() { return _registry; }
    /// The selector engine behind `find` and `findAll`, to extend with type resolvers and ready checks.
    SelectorEngine &selectors() { return _selectors; }

    /// True if `target` is a QObject with a slot or Q_INVOKABLE named `name`, or a MethodNameHook says so.
    bool isMethod(const Target &target, const QString &name) const;

private:
    void registerBuiltinMethods();
    QJsonValue get(const QJsonObject &params);
    QJsonValue children(const QJsonObject &params);

    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    QByteArray handleLine(const QByteArray &line);

    Registry _registry;
    SelectorEngine _selectors{_registry};
    QHash<QString, Method> _methods;
    std::vector<PropertyHook> _propertyHooks;
    std::vector<MethodNameHook> _methodNameHooks;
    std::vector<ChildrenHook> _childrenHooks;

    QLocalServer *_server = nullptr;
    QPointer<QLocalSocket> _socket;
    QByteArray _buffer;
    bool _processing = false;
};

/// The string parameter `key`; throws BadRequest if it is missing or not a string.
QString stringParam(const QJsonObject &params, const QString &key);

} // namespace QtGuiProbe
