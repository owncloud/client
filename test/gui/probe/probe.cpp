// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#include "probe.h"

#include "actions.h"
#include "items.h"
#include "quick.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QMetaMethod>
#include <QTimer>

#include <exception>
#include <memory>

namespace {

Q_LOGGING_CATEGORY(lcProbe, "qtguiprobe", QtInfoMsg)

const char envVar[] = "QT_GUI_TEST_PROBE";

QPointer<QtGuiProbe::Probe> runningProbe;

// Starts the probe once the event loop runs. Inert unless the environment variable is set.
void startProbeLater()
{
    if (qEnvironmentVariableIsEmpty(envVar)) {
        return;
    }
    QTimer::singleShot(0, QCoreApplication::instance(), [] { QtGuiProbe::Probe::startIfRequested(); });
}

QJsonObject errorObject(const QtGuiProbe::ProbeError &error)
{
    QJsonObject object = error.extra;
    object.insert(QStringLiteral("code"), error.code);
    object.insert(QStringLiteral("message"), error.message);
    return object;
}

QtGuiProbe::ProbeError badRequest(const QString &message)
{
    return {QStringLiteral("BadRequest"), message};
}

QJsonObject selectorParam(const QJsonObject &params)
{
    const QJsonValue value = params.value(QStringLiteral("selector"));
    if (!value.isObject()) {
        throw badRequest(QStringLiteral("parameter 'selector' must be an object"));
    }
    return value.toObject();
}

// Clears a flag when it goes out of scope, whatever way the scope is left.
class FlagGuard
{
public:
    explicit FlagGuard(bool &flag)
        : _flag(flag)
    {
        _flag = true;
    }
    ~FlagGuard() { _flag = false; }
    FlagGuard(const FlagGuard &) = delete;
    FlagGuard &operator=(const FlagGuard &) = delete;

private:
    bool &_flag;
};

} // namespace

Q_COREAPP_STARTUP_FUNCTION(startProbeLater)

namespace QtGuiProbe {

QString stringParam(const QJsonObject &params, const QString &key)
{
    const QJsonValue value = params.value(key);
    if (!value.isString()) {
        throw badRequest(QStringLiteral("parameter '%1' must be a string").arg(key));
    }
    return value.toString();
}

Probe::Probe(QObject *parent)
    : QObject(parent)
{
    registerBuiltinMethods();
}

Probe::~Probe() = default;

Probe *Probe::startIfRequested()
{
    const QString name = qEnvironmentVariable(envVar);
    if (name.isEmpty()) {
        return nullptr;
    }
    if (runningProbe) {
        return runningProbe;
    }
    auto *probe = new Probe(QCoreApplication::instance());
    if (!probe->listen(name)) {
        delete probe;
        return nullptr;
    }
    runningProbe = probe;
    return probe;
}

bool Probe::listen(const QString &name)
{
    if (!_server) {
        _server = new QLocalServer(this);
        _server->setSocketOptions(QLocalServer::UserAccessOption);
        connect(_server, &QLocalServer::newConnection, this, &Probe::onNewConnection);
    }
    _server->close();
    // a crashed earlier run may have left its socket behind
    QLocalServer::removeServer(name);
    if (!_server->listen(name)) {
        qCWarning(lcProbe) << "cannot listen on" << name << ":" << _server->errorString();
        return false;
    }
    qCInfo(lcProbe) << "listening on" << _server->fullServerName();
    return true;
}

void Probe::registerMethod(const QString &name, Method method)
{
    _methods.insert(name, std::move(method));
}

void Probe::registerPropertyHook(PropertyHook hook)
{
    _propertyHooks.push_back(std::move(hook));
}

void Probe::registerMethodNameHook(MethodNameHook hook)
{
    _methodNameHooks.push_back(std::move(hook));
}

void Probe::registerChildrenHook(ChildrenHook hook)
{
    _childrenHooks.push_back(std::move(hook));
}

void Probe::registerBuiltinMethods()
{
    registerMethod(QStringLiteral("hello"), [](const QJsonObject &) -> QJsonValue {
        return QJsonObject{{QStringLiteral("pid"), QCoreApplication::applicationPid()}, {QStringLiteral("qtVersion"), QString::fromLatin1(qVersion())}};
    });
    registerMethod(QStringLiteral("get"), [this](const QJsonObject &params) { return get(params); });
    registerMethod(QStringLiteral("children"), [this](const QJsonObject &params) { return children(params); });
    registerMethod(QStringLiteral("release"), [this](const QJsonObject &) -> QJsonValue {
        _registry.clear();
        return QJsonValue::Null;
    });
    registerMethod(QStringLiteral("find"), [this](const QJsonObject &params) -> QJsonValue {
        const QJsonValue ready = params.value(QStringLiteral("ready"));
        if (!ready.isUndefined() && !ready.isNull() && !ready.isBool()) {
            throw badRequest(QStringLiteral("parameter 'ready' must be a bool"));
        }
        const auto target = _selectors.find(selectorParam(params), ready.toBool());
        return target ? _registry.ref(*target) : Registry::nullRef();
    });
    registerMethod(QStringLiteral("findAll"), [this](const QJsonObject &params) -> QJsonValue {
        QJsonArray result;
        for (const auto &target : _selectors.findAll(selectorParam(params))) {
            result.append(_registry.ref(target));
        }
        return result;
    });
    // `get` and selector keys read properties in the same order: the object's own properties, then the property
    // hooks, then the readers of the item types, so the hooks are registered first
    _selectors.addPropertyReader([this](const Target &target, const QString &name) -> std::optional<QJsonValue> {
        for (const auto &hook : _propertyHooks) {
            if (auto value = hook(target, name)) {
                return value;
            }
        }
        return std::nullopt;
    });
    // model indexes, tabs and header sections
    registerItemSupport(_selectors, _registry);
    // Qt Quick items in a QQuickWidget
    registerQuickSupport(_selectors, _registry);
    // call, input, clipboard, screenshot
    registerActions(*this);
}

QJsonObject Probe::handle(const QJsonObject &request)
{
    QJsonObject response{{QStringLiteral("id"), request.contains(QStringLiteral("id")) ? request.value(QStringLiteral("id")) : QJsonValue::Null}};
    try {
        const QJsonValue methodValue = request.value(QStringLiteral("method"));
        if (!methodValue.isString()) {
            throw badRequest(QStringLiteral("the request has no method"));
        }
        const QString method = methodValue.toString();
        const auto it = _methods.constFind(method);
        if (it == _methods.cend()) {
            throw badRequest(QStringLiteral("unknown method %1").arg(method));
        }
        const QJsonValue params = request.value(QStringLiteral("params"));
        if (!params.isUndefined() && !params.isNull() && !params.isObject()) {
            throw badRequest(QStringLiteral("params must be an object"));
        }
        const QJsonValue result = it.value()(params.toObject());
        response.insert(QStringLiteral("result"), result.isUndefined() ? QJsonValue::Null : result);
    } catch (const ProbeError &error) {
        response.insert(QStringLiteral("error"), errorObject(error));
    } catch (const std::exception &error) {
        response.insert(QStringLiteral("error"), errorObject(badRequest(QStringLiteral("internal error: %1").arg(QString::fromLocal8Bit(error.what())))));
    } catch (...) {
        response.insert(QStringLiteral("error"), errorObject(badRequest(QStringLiteral("internal error: unknown exception"))));
    }
    return response;
}

bool Probe::isMethod(const Target &target, const QString &name) const
{
    if (const auto *object = std::get_if<QObject *>(&target)) {
        const QByteArray key = name.toUtf8();
        const QMetaObject *metaObject = (*object)->metaObject();
        for (int i = 0; i < metaObject->methodCount(); ++i) {
            const QMetaMethod method = metaObject->method(i);
            if ((method.methodType() == QMetaMethod::Slot || method.methodType() == QMetaMethod::Method) && method.name() == key) {
                return true;
            }
        }
    }
    for (const auto &hook : _methodNameHooks) {
        if (hook(target, name)) {
            return true;
        }
    }
    return false;
}

QJsonValue Probe::get(const QJsonObject &params)
{
    const Target target = _registry.resolve(params.value(QStringLiteral("handle")));
    const QString name = stringParam(params, QStringLiteral("name"));
    if (auto value = _selectors.readValue(target, name)) {
        return *value;
    }
    throw ProbeError{QStringLiteral("NoSuchProperty"), QStringLiteral("%1 has no property %2").arg(_registry.className(target), name),
        QJsonObject{{QStringLiteral("isMethod"), isMethod(target, name)}}};
}

QJsonValue Probe::children(const QJsonObject &params)
{
    const Target target = _registry.resolve(params.value(QStringLiteral("handle")));
    // a view lists its items first, then its widgets
    QList<Target> children = itemChildren(target);
    if (const auto *object = std::get_if<QObject *>(&target)) {
        const QObjectList objectChildren = _selectors.childrenOf(*object);
        for (auto *child : objectChildren) {
            children.append(child);
        }
    }
    for (const auto &hook : _childrenHooks) {
        hook(target, children);
    }
    QJsonArray result;
    for (const auto &child : std::as_const(children)) {
        result.append(_registry.ref(child));
    }
    return result;
}

void Probe::connectTransport(Writer writer)
{
    _writer = std::move(writer);
    ++_connection;
    _buffer.clear();
}

void Probe::receive(const QByteArray &bytes)
{
    _buffer.append(bytes);
    processBuffer();
}

void Probe::postInput(std::function<void()> input)
{
    _pendingInput.push_back(std::move(input));
    if (!_inputScheduled) {
        _inputScheduled = true;
        QMetaObject::invokeMethod(this, &Probe::runNextInput, Qt::QueuedConnection);
    }
}

void Probe::runNextInput()
{
    _inputScheduled = false;
    if (_pendingInput.empty()) {
        return;
    }
    const std::function<void()> input = std::move(_pendingInput.front());
    _pendingInput.pop_front();
    const auto running = std::make_shared<bool>(true);
    if (!_pendingInput.empty()) {
        // the next input runs after this one, or inside the event loop of a dialog this one opens
        _inputScheduled = true;
        QMetaObject::invokeMethod(this, &Probe::runNextInput, Qt::QueuedConnection);
    } else {
        // if this input opens a modal dialog, its event loop serves the waiting requests
        QMetaObject::invokeMethod(
            this,
            [this, running] {
                if (*running) {
                    processBuffer();
                }
            },
            Qt::QueuedConnection);
    }
    try {
        input();
    } catch (const ProbeError &error) {
        qCWarning(lcProbe) << "input failed:" << error.code << error.message;
    } catch (const std::exception &error) {
        qCWarning(lcProbe) << "input failed:" << error.what();
    } catch (...) {
        qCWarning(lcProbe) << "input failed with an unknown exception";
    }
    *running = false;
    if (_pendingInput.empty()) {
        // after the events the input posted itself
        QMetaObject::invokeMethod(this, &Probe::processBuffer, Qt::QueuedConnection);
    }
}

void Probe::onNewConnection()
{
    while (QLocalSocket *socket = _server->nextPendingConnection()) {
        if (_socket) {
            qCWarning(lcProbe) << "refusing a second connection";
            socket->abort();
            socket->deleteLater();
            continue;
        }
        _socket = socket;
        connectTransport([socket = QPointer<QLocalSocket>(socket)](const QByteArray &line) {
            if (socket) {
                socket->write(line);
            }
        });
        connect(socket, &QLocalSocket::readyRead, this, &Probe::onReadyRead);
        connect(socket, &QLocalSocket::disconnected, this, &Probe::onDisconnected);
        // data may have arrived before the connections above
        if (socket->bytesAvailable() > 0) {
            QTimer::singleShot(0, this, &Probe::onReadyRead);
        }
    }
}

void Probe::onReadyRead()
{
    if (_socket) {
        receive(_socket->readAll());
    }
}

void Probe::processBuffer()
{
    // a method that spins an event loop re-enters here; the outer loop then takes the new lines
    if (_processing) {
        return;
    }
    const FlagGuard processing(_processing);
    qsizetype end;
    // a request after posted input waits until that input has run (see postInput)
    while (_writer && _pendingInput.empty() && (end = _buffer.indexOf('\n')) >= 0) {
        const QByteArray line = _buffer.left(end);
        _buffer.remove(0, end + 1);
        if (line.trimmed().isEmpty()) {
            continue;
        }
        const quint64 connection = _connection;
        const QByteArray response = handleLine(line);
        // the client may have gone, or been replaced, while the request ran
        if (connection == _connection && _writer) {
            _writer(response);
        }
    }
}

void Probe::onDisconnected()
{
    if (_socket) {
        _socket->deleteLater();
        _socket = nullptr;
    }
    _writer = {};
    ++_connection;
    _buffer.clear();
    _registry.clear();
}

QByteArray Probe::handleLine(const QByteArray &line)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
    QJsonObject response;
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        const QString reason = parseError.error != QJsonParseError::NoError ? parseError.errorString() : QStringLiteral("not a JSON object");
        response = QJsonObject{{QStringLiteral("id"), QJsonValue::Null},
            {QStringLiteral("error"), errorObject(badRequest(QStringLiteral("cannot parse the request: %1").arg(reason)))}};
    } else {
        response = handle(document.object());
    }
    return QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n';
}

} // namespace QtGuiProbe
