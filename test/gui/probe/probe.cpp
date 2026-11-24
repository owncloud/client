// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#include "probe.h"

#include "items.h"
#include "jsonvalue.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QTimer>

#include <exception>

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

std::optional<QVariant> readObjectProperty(const QObject *object, const QString &name)
{
    const QByteArray key = name.toUtf8();
    const QMetaObject *metaObject = object->metaObject();
    const int index = metaObject->indexOfProperty(key.constData());
    if (index >= 0) {
        return metaObject->property(index).read(object);
    }
    if (object->dynamicPropertyNames().contains(key)) {
        return object->property(key.constData());
    }
    if (name == QLatin1String("objectName")) {
        return object->objectName();
    }
    return std::nullopt;
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
    // model indexes, tabs and header sections
    registerItemSupport(_selectors, _registry);
    // property hooks serve selector keys as they serve `get`
    _selectors.addPropertyReader([this](const Target &target, const QString &name) -> std::optional<QJsonValue> {
        for (const auto &hook : _propertyHooks) {
            if (auto value = hook(target, name)) {
                return value;
            }
        }
        return std::nullopt;
    });
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
    if (const auto *object = std::get_if<QObject *>(&target)) {
        if (const auto value = readObjectProperty(*object, name)) {
            return toJson(*value, _registry);
        }
    }
    for (const auto &hook : _propertyHooks) {
        if (auto value = hook(target, name)) {
            return *value;
        }
    }
    if (auto value = itemProperty(target, name)) {
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
        const QObjectList objectChildren = searchChildren(*object);
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
        _buffer.clear();
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
    if (!_socket) {
        return;
    }
    _buffer.append(_socket->readAll());
    // a method that spins an event loop (a modal dialog) re-enters here; the outer loop then takes the new lines
    if (_processing) {
        return;
    }
    const FlagGuard processing(_processing);
    qsizetype end;
    while (_socket && (end = _buffer.indexOf('\n')) >= 0) {
        const QByteArray line = _buffer.left(end);
        _buffer.remove(0, end + 1);
        if (line.trimmed().isEmpty()) {
            continue;
        }
        const QPointer<QLocalSocket> socket = _socket;
        const QByteArray response = handleLine(line);
        // the client may have gone, or been replaced, while the request ran
        if (socket && socket == _socket) {
            socket->write(response);
        }
    }
}

void Probe::onDisconnected()
{
    if (_socket) {
        _socket->deleteLater();
        _socket = nullptr;
    }
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
