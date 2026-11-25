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

#include <deque>
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

    /// Answers one request object; the socket handler calls this for every line. It does not wait for posted input.
    QJsonObject handle(const QJsonObject &request);

    /// Receives one response line (with its trailing newline).
    using Writer = std::function<void(const QByteArray &line)>;
    /// Serves a client that is not on the socket (tests, other transports): `writer` gets the response lines to the
    /// requests passed to receive(). Replaces the current client, as a new socket connection would.
    void connectTransport(Writer writer);
    /// Request bytes from the client, split into lines and answered as on the socket.
    void receive(const QByteArray &bytes);

    /// Runs `input` (a click, key strokes, an activation) from the event loop instead of inside the request, so the
    /// request is answered even if the input opens a modal dialog. Inputs run in order. Requests the client sends
    /// after one wait until all posted input has run or is running in a nested event loop (a modal dialog), so a
    /// later request sees the input's effects. The request that posted it has been answered already: a failure
    /// inside `input` (its target gone, no widget, an exception, which is caught) is only logged, to the
    /// application's log, and the client notices it only as a later timeout.
    void postInput(std::function<void()> input);

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
    void processBuffer();
    void runNextInput();
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
    Writer _writer;
    /// Changes with every new or lost client, so a response to a request of the previous client is dropped.
    quint64 _connection = 0;
    bool _processing = false;
    std::deque<std::function<void()>> _pendingInput;
    bool _inputScheduled = false;
};

/// The string parameter `key`; throws BadRequest if it is missing or not a string.
QString stringParam(const QJsonObject &params, const QString &key);

} // namespace QtGuiProbe
