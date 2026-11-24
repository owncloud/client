// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#include "jsonvalue.h"
#include "probe.h"
#include "registry.h"

#include <QAction>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMenu>
#include <QPushButton>
#include <QRect>
#include <QSize>
#include <QStandardItemModel>
#include <QTest>
#include <QTimeZone>
#include <QUrl>

using namespace QtGuiProbe;

namespace {

QJsonObject request(Probe &probe, const QString &method, const QJsonObject &params = {})
{
    return probe.handle(QJsonObject{{QStringLiteral("id"), 7}, {QStringLiteral("method"), method}, {QStringLiteral("params"), params}});
}

QString errorCode(const QJsonObject &response)
{
    return response.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString();
}

QJsonValue result(const QJsonObject &response)
{
    if (response.contains(QStringLiteral("error"))) {
        qWarning() << "unexpected error" << response;
    }
    return response.value(QStringLiteral("result"));
}

QJsonObject getRequest(const QJsonValue &handle, const QString &name)
{
    return {{QStringLiteral("handle"), handle}, {QStringLiteral("name"), name}};
}

QString socketName(const QString &tag)
{
    // short: Unix socket paths are limited to about 100 bytes
    return QDir::tempPath() + QStringLiteral("/qgp-%1-%2").arg(tag).arg(QCoreApplication::applicationPid());
}

QJsonObject readResponse(QLocalSocket &socket)
{
    if (!QTest::qWaitFor([&socket] { return socket.canReadLine(); }, 5000)) {
        return {};
    }
    const QByteArray line = socket.readLine();
    if (!line.endsWith('\n')) {
        return {};
    }
    return QJsonDocument::fromJson(line).object();
}

} // namespace

class TestProtocol : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void helloReturnsPid()
    {
        Probe probe;
        const QJsonObject response = request(probe, QStringLiteral("hello"));
        QCOMPARE(response.value(QStringLiteral("id")).toInt(), 7);
        const QJsonObject hello = result(response).toObject();
        QCOMPARE(hello.value(QStringLiteral("pid")).toInteger(), QCoreApplication::applicationPid());
        QCOMPARE(hello.value(QStringLiteral("qtVersion")).toString(), QString::fromLatin1(qVersion()));
    }

    void getReturnsUnicodeText()
    {
        Probe probe;
        QLabel label(QStringLiteral("Grüße 🙂"));
        const QJsonObject ref = probe.registry().ref(&label);
        QCOMPARE(ref.value(QStringLiteral("$class")).toString(), QStringLiteral("QLabel"));
        const QJsonObject response = request(probe, QStringLiteral("get"), getRequest(ref, QStringLiteral("text")));
        QCOMPARE(result(response), QJsonValue(QStringLiteral("Grüße 🙂")));
    }

    void getReadsDynamicPropertiesAndObjectName()
    {
        Probe probe;
        QLabel label;
        label.setObjectName(QStringLiteral("greeting"));
        label.setProperty("custom", 42);
        const QJsonObject ref = probe.registry().ref(&label);
        QCOMPARE(result(request(probe, QStringLiteral("get"), getRequest(ref, QStringLiteral("custom")))), QJsonValue(42));
        QCOMPARE(result(request(probe, QStringLiteral("get"), getRequest(ref, QStringLiteral("objectName")))), QJsonValue(QStringLiteral("greeting")));
    }

    void getUnknownPropertyReportsIsMethod()
    {
        Probe probe;
        QPushButton button;
        const QJsonObject ref = probe.registry().ref(&button);

        QJsonObject error = request(probe, QStringLiteral("get"), getRequest(ref, QStringLiteral("click"))).value(QStringLiteral("error")).toObject();
        QCOMPARE(error.value(QStringLiteral("code")).toString(), QStringLiteral("NoSuchProperty"));
        QCOMPARE(error.value(QStringLiteral("isMethod")), QJsonValue(true));

        error = request(probe, QStringLiteral("get"), getRequest(ref, QStringLiteral("noSuchThing"))).value(QStringLiteral("error")).toObject();
        QCOMPARE(error.value(QStringLiteral("code")).toString(), QStringLiteral("NoSuchProperty"));
        QCOMPARE(error.value(QStringLiteral("isMethod")), QJsonValue(false));
        QVERIFY(!error.value(QStringLiteral("message")).toString().isEmpty());

        // signals are not callable
        error = request(probe, QStringLiteral("get"), getRequest(ref, QStringLiteral("clicked"))).value(QStringLiteral("error")).toObject();
        QCOMPARE(error.value(QStringLiteral("isMethod")), QJsonValue(false));
    }

    void hooksExtendGet()
    {
        Probe probe;
        QLabel label;
        probe.registerPropertyHook([](const Target &target, const QString &name) -> std::optional<QJsonValue> {
            if (std::holds_alternative<QObject *>(target) && name == QLatin1String("answer")) {
                return QJsonValue(42);
            }
            return std::nullopt;
        });
        probe.registerMethodNameHook([](const Target &, const QString &name) { return name == QLatin1String("allowed"); });
        const QJsonObject ref = probe.registry().ref(&label);
        QCOMPARE(result(request(probe, QStringLiteral("get"), getRequest(ref, QStringLiteral("answer")))), QJsonValue(42));
        // a real property wins over hooks
        QCOMPARE(result(request(probe, QStringLiteral("get"), getRequest(ref, QStringLiteral("text")))), QJsonValue(QString()));
        const QJsonObject error = request(probe, QStringLiteral("get"), getRequest(ref, QStringLiteral("allowed"))).value(QStringLiteral("error")).toObject();
        QCOMPARE(error.value(QStringLiteral("isMethod")), QJsonValue(true));
    }

    void unknownMethodIsBadRequest()
    {
        Probe probe;
        QCOMPARE(errorCode(request(probe, QStringLiteral("noSuchMethod"))), QStringLiteral("BadRequest"));
        QCOMPARE(errorCode(probe.handle(QJsonObject{{QStringLiteral("id"), 1}})), QStringLiteral("BadRequest"));
    }

    void missingParamsAreBadRequest()
    {
        Probe probe;
        QLabel label;
        const QJsonObject ref = probe.registry().ref(&label);
        QCOMPARE(errorCode(request(probe, QStringLiteral("get"), {{QStringLiteral("handle"), ref}})), QStringLiteral("BadRequest"));
        QCOMPARE(errorCode(request(probe, QStringLiteral("get"), {{QStringLiteral("name"), QStringLiteral("text")}})), QStringLiteral("BadRequest"));
    }

    void deletedWidgetIsDeadObject()
    {
        Probe probe;
        auto *label = new QLabel(QStringLiteral("bye"));
        const QJsonObject ref = probe.registry().ref(label);
        delete label;
        QCOMPARE(errorCode(request(probe, QStringLiteral("get"), getRequest(ref, QStringLiteral("text")))), QStringLiteral("DeadObject"));
    }

    void sameObjectKeepsItsHandle()
    {
        Probe probe;
        QLabel label;
        QCOMPARE(probe.registry().ref(&label), probe.registry().ref(&label));
        QCOMPARE(probe.registry().ref(static_cast<QObject *>(nullptr)), QJsonObject({{QStringLiteral("$handle"), QJsonValue::Null}}));
    }

    void nullAndUnknownHandles()
    {
        Probe probe;
        QCOMPARE(errorCode(request(probe, QStringLiteral("get"), getRequest(Registry::nullRef(), QStringLiteral("text")))), QStringLiteral("NoSuchObject"));
        QCOMPARE(errorCode(request(probe, QStringLiteral("get"), getRequest(QJsonObject{{QStringLiteral("$handle"), 999}}, QStringLiteral("text")))),
            QStringLiteral("NoSuchObject"));
        QCOMPARE(
            errorCode(request(probe, QStringLiteral("get"), getRequest(QStringLiteral("nonsense"), QStringLiteral("text")))), QStringLiteral("BadRequest"));
    }

    void childrenReturnsChildClasses()
    {
        Probe probe;
        QWidget parent;
        new QLabel(&parent);
        new QPushButton(&parent);
        const QJsonObject ref = probe.registry().ref(&parent);
        const QJsonArray children = result(request(probe, QStringLiteral("children"), {{QStringLiteral("handle"), ref}})).toArray();
        QStringList classes;
        for (const auto &child : children) {
            classes << child.toObject().value(QStringLiteral("$class")).toString();
        }
        QCOMPARE(classes, QStringList({QStringLiteral("QLabel"), QStringLiteral("QPushButton")}));
    }

    void menuChildrenIncludeActions()
    {
        Probe probe;
        QMenu menu;
        QAction foreign(QStringLiteral("foreign")); // not a child of the menu
        menu.addAction(&foreign);
        const QJsonObject ref = probe.registry().ref(&menu);
        const QJsonArray children = result(request(probe, QStringLiteral("children"), {{QStringLiteral("handle"), ref}})).toArray();
        QVERIFY(children.contains(probe.registry().ref(&foreign)));
        // listed once even when the action is also a QObject child
        QAction *own = menu.addAction(QStringLiteral("own"));
        const QJsonArray again = result(request(probe, QStringLiteral("children"), {{QStringLiteral("handle"), ref}})).toArray();
        QCOMPARE(std::count(again.begin(), again.end(), QJsonValue(probe.registry().ref(own))), 1);
    }

    void releaseDropsHandles()
    {
        Probe probe;
        QLabel label;
        const QJsonObject ref = probe.registry().ref(&label);
        QCOMPARE(result(request(probe, QStringLiteral("release"))), QJsonValue(QJsonValue::Null));
        QCOMPARE(errorCode(request(probe, QStringLiteral("get"), getRequest(ref, QStringLiteral("text")))), QStringLiteral("NoSuchObject"));
        // a new ref gets a new handle
        QVERIFY(probe.registry().ref(&label) != ref);
    }

    void registryRefsItems()
    {
        Registry registry;
        QStandardItemModel model(2, 1);
        QJsonObject ref = registry.ref(model.index(1, 0));
        QCOMPARE(ref.value(QStringLiteral("$class")).toString(), QStringLiteral("QModelIndex"));
        QVERIFY(std::holds_alternative<QPersistentModelIndex>(registry.resolve(ref)));
        model.removeRow(1);
        QVERIFY_THROWS_EXCEPTION(ProbeError, registry.resolve(ref));

        auto *owner = new QObject;
        ref = registry.ref(SyntheticItem{owner, QStringLiteral("TabItem"), 3});
        QCOMPARE(ref.value(QStringLiteral("$class")).toString(), QStringLiteral("TabItem"));
        QCOMPARE(std::get<SyntheticItem>(registry.resolve(ref)).index, 3);
        delete owner;
        try {
            registry.resolve(ref);
            QFAIL("expected DeadObject");
        } catch (const ProbeError &error) {
            QCOMPARE(error.code, QStringLiteral("DeadObject"));
        }
    }

    void registryClassNameHook()
    {
        Registry registry;
        registry.registerClassNameHook(
            [](const QObject *object) { return object->objectName() == QLatin1String("item") ? QStringLiteral("Item") : QString(); });
        QObject item;
        item.setObjectName(QStringLiteral("item"));
        QObject other;
        QCOMPARE(registry.ref(&item).value(QStringLiteral("$class")).toString(), QStringLiteral("Item"));
        QCOMPARE(registry.ref(&other).value(QStringLiteral("$class")).toString(), QStringLiteral("QObject"));
    }

    void toJsonConvertsValues()
    {
        Registry registry;
        QLabel label;
        QCOMPARE(toJson(QVariant(), registry), QJsonValue(QJsonValue::Null));
        QCOMPARE(toJson(QStringLiteral("ä"), registry), QJsonValue(QStringLiteral("ä")));
        QCOMPARE(toJson(3, registry), QJsonValue(3));
        QCOMPARE(toJson(2.5, registry), QJsonValue(2.5));
        QCOMPARE(toJson(true, registry), QJsonValue(true));
        QCOMPARE(toJson(QStringList({QStringLiteral("a"), QStringLiteral("b")}), registry), QJsonValue(QJsonArray({QStringLiteral("a"), QStringLiteral("b")})));
        QCOMPARE(toJson(QVariantList({1, QStringLiteral("x")}), registry), QJsonValue(QJsonArray({1, QStringLiteral("x")})));
        QCOMPARE(toJson(QVariantMap({{QStringLiteral("k"), 1}}), registry), QJsonValue(QJsonObject({{QStringLiteral("k"), 1}})));
        QCOMPARE(toJson(QUrl(QStringLiteral("https://example.org/a b")), registry), QJsonValue(QStringLiteral("https://example.org/a b")));
        const QDateTime when(QDate(2026, 11, 24), QTime(12, 30, 5), QTimeZone::UTC);
        QCOMPARE(toJson(when, registry), QJsonValue(QStringLiteral("2026-11-24T12:30:05Z")));
        QCOMPARE(toJson(QSize(3, 4), registry), QJsonValue(QJsonObject({{QStringLiteral("width"), 3}, {QStringLiteral("height"), 4}})));
        QCOMPARE(toJson(QRect(1, 2, 3, 4), registry),
            QJsonValue(QJsonObject({{QStringLiteral("x"), 1}, {QStringLiteral("y"), 2}, {QStringLiteral("width"), 3}, {QStringLiteral("height"), 4}})));
        QCOMPARE(toJson(QVariant::fromValue(static_cast<QObject *>(&label)), registry), QJsonValue(registry.ref(&label)));
        QCOMPARE(toJson(QVariant::fromValue(&label), registry), QJsonValue(registry.ref(&label)));
        QCOMPARE(toJson(QVariant::fromValue(static_cast<QObject *>(nullptr)), registry), QJsonValue(Registry::nullRef()));
        QCOMPARE(toJson(QVariant::fromValue(Qt::AlignRight), registry), QJsonValue(static_cast<int>(Qt::AlignRight)));
        QCOMPARE(
            toJson(QVariant::fromValue(Qt::Alignment(Qt::AlignRight | Qt::AlignTop)), registry), QJsonValue(static_cast<int>(Qt::AlignRight | Qt::AlignTop)));
        QCOMPARE(toJson(QVariant::fromValue(QList<QObject *>({&label})), registry), QJsonValue(QJsonArray({registry.ref(&label)})));
    }

    void fromJsonResolvesHandles()
    {
        Registry registry;
        QLabel label;
        QCOMPARE(fromJson(registry.ref(&label), registry).value<QObject *>(), &label);
        QCOMPARE(fromJson(QJsonValue(QStringLiteral("x")), registry), QVariant(QStringLiteral("x")));
        QCOMPARE(fromJson(QJsonArray({1, registry.ref(&label)}), registry).toList().at(1).value<QObject *>(), &label);
    }

    void listenReplacesStaleServer()
    {
        const QString name = socketName(QStringLiteral("stale"));
        QLocalServer stale;
        QVERIFY2(stale.listen(name), qPrintable(stale.errorString()));
        // stale is left listening, as by a crashed earlier run
        Probe probe;
        QVERIFY(probe.listen(name));
        // clients reach the probe, not the stale server
        QLocalSocket socket;
        socket.connectToServer(name);
        QVERIFY(socket.waitForConnected(5000));
        socket.write(R"({"id": 1, "method": "hello"})"
                     "\n");
        QCOMPARE(readResponse(socket).value(QStringLiteral("id")).toInt(), 1);
        QVERIFY(!stale.hasPendingConnections());
    }

    void startIfRequestedWithoutEnvironment()
    {
        qunsetenv("QT_GUI_TEST_PROBE");
        QCOMPARE(Probe::startIfRequested(), nullptr);
    }

    void servesLinesOverTheSocket()
    {
        const QString name = socketName(QStringLiteral("sock"));
        qputenv("QT_GUI_TEST_PROBE", name.toLocal8Bit());
        Probe *probe = Probe::startIfRequested();
        qunsetenv("QT_GUI_TEST_PROBE");
        QVERIFY(probe);
        QLabel label(QStringLiteral("Grüße 🙂"));
        const QJsonObject ref = probe->registry().ref(&label);

        QLocalSocket socket;
        socket.connectToServer(name);
        QVERIFY(socket.waitForConnected(5000));

        // two requests in one write, the second one split over two writes
        const QByteArray get = QJsonDocument(QJsonObject{{QStringLiteral("id"), 2}, {QStringLiteral("method"), QStringLiteral("get")},
                                                 {QStringLiteral("params"), getRequest(ref, QStringLiteral("text"))}})
                                   .toJson(QJsonDocument::Compact);
        socket.write(R"({"id": 1, "method": "hello"})"
                     "\n"
            + get.left(10));
        socket.flush();
        QJsonObject response = readResponse(socket);
        QCOMPARE(response.value(QStringLiteral("id")).toInt(), 1);
        QCOMPARE(response.value(QStringLiteral("result")).toObject().value(QStringLiteral("pid")).toInteger(), QCoreApplication::applicationPid());
        socket.write(get.mid(10) + "\n");
        response = readResponse(socket);
        QCOMPARE(response.value(QStringLiteral("id")).toInt(), 2);
        QCOMPARE(response.value(QStringLiteral("result")).toString(), QStringLiteral("Grüße 🙂"));

        // an unparsable line is answered with a null id
        socket.write("{nonsense\n");
        response = readResponse(socket);
        QVERIFY(response.contains(QStringLiteral("id")));
        QVERIFY(response.value(QStringLiteral("id")).isNull());
        QCOMPARE(errorCode(response), QStringLiteral("BadRequest"));

        // a second connection is refused while the first one is open
        QLocalSocket second;
        second.connectToServer(name);
        QVERIFY(QTest::qWaitFor([&second] { return second.state() == QLocalSocket::UnconnectedState; }, 5000));

        // closing the connection drops the handles; the next client can connect
        socket.disconnectFromServer();
        QVERIFY(QTest::qWaitFor([&probe, &ref] {
            try {
                probe->registry().resolve(ref);
                return false;
            } catch (const ProbeError &) {
                return true;
            }
        }));
        QLocalSocket third;
        third.connectToServer(name);
        QVERIFY(third.waitForConnected(5000));
        third.write(R"({"id": 3, "method": "hello"})"
                    "\n");
        QCOMPARE(readResponse(third).value(QStringLiteral("id")).toInt(), 3);
        QCOMPARE(Probe::startIfRequested(), nullptr); // env var gone again
        delete probe;
    }
};

QTEST_MAIN(TestProtocol)
#include "tst_protocol.moc"
