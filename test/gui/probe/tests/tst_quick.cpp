// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#include "items.h"
#include "probe.h"
#include "quick.h"
#include "registry.h"
#include "selector.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QQuickItem>
#include <QQuickWidget>
#include <QTabBar>
#include <QTest>
#include <QVBoxLayout>

#include <memory>

using namespace QtGuiProbe;

// Stand-ins for the class names the QML engine and Qt Quick give their types.
class QQuickProbeFixtureItem : public QObject
{
    Q_OBJECT
};

class ProbeFixtureDelegate_QMLTYPE_3 : public QObject
{
    Q_OBJECT
};

class ProbeFixtureRow_QML_12 : public QObject
{
    Q_OBJECT
};

namespace {

QJsonObject json(const char *text)
{
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray(text));
    Q_ASSERT(document.isObject());
    return document.object();
}

QJsonObject request(Probe &probe, const QString &method, const QJsonObject &params = {})
{
    return probe.handle(QJsonObject{{QStringLiteral("id"), 1}, {QStringLiteral("method"), method}, {QStringLiteral("params"), params}});
}

QJsonValue result(const QJsonObject &response)
{
    if (response.contains(QStringLiteral("error"))) {
        qWarning() << "error response" << response;
    }
    return response.value(QStringLiteral("result"));
}

QJsonObject find(Probe &probe, const QJsonObject &selector, bool ready = false)
{
    return result(request(probe, QStringLiteral("find"), {{QStringLiteral("selector"), selector}, {QStringLiteral("ready"), ready}})).toObject();
}

QJsonArray findAll(Probe &probe, const QJsonObject &selector)
{
    return result(request(probe, QStringLiteral("findAll"), {{QStringLiteral("selector"), selector}})).toArray();
}

QJsonValue get(Probe &probe, const QJsonObject &ref, const char *name)
{
    return result(request(probe, QStringLiteral("get"), {{QStringLiteral("handle"), ref}, {QStringLiteral("name"), QString::fromLatin1(name)}}));
}

QJsonArray childrenOf(Probe &probe, const QJsonObject &ref)
{
    return result(request(probe, QStringLiteral("children"), {{QStringLiteral("handle"), ref}})).toArray();
}

bool isNull(const QJsonObject &ref)
{
    return ref.value(QStringLiteral("$handle")).isNull();
}

QObject *objectOf(const Target &target)
{
    const auto *object = std::get_if<QObject *>(&target);
    return object ? *object : nullptr;
}

// A window holding a QQuickWidget named "quickWidget" that shows tst_quick.qml.
struct QuickFixture
{
    QWidget window;
    QQuickWidget *quickWidget = new QQuickWidget(&window);

    QuickFixture()
    {
        window.setObjectName(QStringLiteral("window"));
        quickWidget->setObjectName(QStringLiteral("quickWidget"));
        quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
        (new QVBoxLayout(&window))->addWidget(quickWidget);
        quickWidget->setSource(QUrl(QStringLiteral("qrc:/tst_quick.qml")));
        window.resize(320, 320);
    }

    bool loaded() const
    {
        if (quickWidget->status() != QQuickWidget::Ready) {
            qWarning() << quickWidget->errors();
            return false;
        }
        return true;
    }

    void show()
    {
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
    }

    // The item with QML id (or object name) `id`.
    QQuickItem *item(const char *id) const
    {
        const auto items = quickWidget->rootObject()->findChildren<QQuickItem *>();
        for (auto *item : items) {
            if (quickId(item) == QLatin1String(id) || item->objectName() == QLatin1String(id)) {
                return item;
            }
        }
        return nullptr;
    }
};

const char quickWidgetSelector[] = R"({"name":"quickWidget","type":"QQuickWidget"})";
const char userNameSelector[] = R"({"container":{"name":"quickWidget","type":"QQuickWidget"},"id":"userNameField","type":"TextField","visible":true})";
const char loginButtonSelector[] = R"({"container":{"name":"quickWidget","type":"QQuickWidget"},"id":"loginButton","type":"Button","visible":true})";
const char labelSelector[] = R"({"container":{"name":"quickWidget","type":"QQuickWidget"},"type":"Label","visible":true})";
const char scrollViewSelector[] =
    R"({"container":{"name":"quickWidget","type":"QQuickWidget"},"id":"scrollView","type":"ScrollView","unnamed":1,"visible":true})";
const char labelInScrollViewSelector[] = R"({"container":{"container":{"name":"quickWidget","type":"QQuickWidget"},"id":"scrollView","type":"ScrollView",
    "unnamed":1,"visible":true},"type":"Label","visible":true,"text":"alice"})";

} // namespace

class TestQuick : public QObject
{
    Q_OBJECT

private:
    Registry _registry;
    std::unique_ptr<SelectorEngine> _engine;

private Q_SLOTS:
    void init()
    {
        _registry.clear();
        _engine = std::make_unique<SelectorEngine>(_registry);
        registerQuickSupport(*_engine, _registry);
    }

    void squishTypeNameStripsQmlSuffixAndQQuickPrefix()
    {
        QQuickProbeFixtureItem quick;
        QCOMPARE(squishTypeName(&quick), QStringLiteral("ProbeFixtureItem"));
        ProbeFixtureDelegate_QMLTYPE_3 delegate;
        QCOMPARE(squishTypeName(&delegate), QStringLiteral("ProbeFixtureDelegate"));
        ProbeFixtureRow_QML_12 row;
        QCOMPARE(squishTypeName(&row), QStringLiteral("ProbeFixtureRow"));
    }

    void suiteTypesHaveTheirQmlNames()
    {
        QuickFixture fixture;
        QVERIFY(fixture.loaded());
        fixture.show();
        const QList<std::pair<const char *, QString>> expected{
            {"page", QStringLiteral("Item")},
            {"scrollView", QStringLiteral("ScrollView")},
            {"userNameField", QStringLiteral("TextField")},
            {"loginButton", QStringLiteral("Button")}, // declares a property, so its meta-object is derived once more
            {"hiddenText", QStringLiteral("Text")},
            {"spaces", QStringLiteral("ListView")},
            {"aliceLabel", QStringLiteral("Label")},
        };
        for (const auto &[id, type] : expected) {
            QObject *object = qstrcmp(id, "page") == 0 ? fixture.quickWidget->rootObject() : fixture.item(id);
            QVERIFY2(object, id);
            QCOMPARE(squishTypeName(object), type);
            QCOMPARE(_registry.className(object), type);
        }
        // widgets keep their class names
        QCOMPARE(_registry.className(fixture.quickWidget), QStringLiteral("QQuickWidget"));
    }

    void resolvesItemsBelowTheQuickWidget()
    {
        QuickFixture fixture;
        QVERIFY(fixture.loaded());
        fixture.show();

        const auto userName = _engine->findAll(json(userNameSelector));
        QCOMPARE(userName.size(), size_t(1));
        QCOMPARE(objectOf(userName.front()), fixture.item("userNameField"));

        const auto scrollView = _engine->findAll(json(scrollViewSelector));
        QCOMPARE(scrollView.size(), size_t(1));
        QCOMPARE(objectOf(scrollView.front()), fixture.item("scrollView"));

        // nested: an item as the container, as SyncConnectionWizard's space selector does
        const auto label = _engine->findAll(json(labelInScrollViewSelector));
        QCOMPARE(label.size(), size_t(1));
        QCOMPARE(objectOf(label.front())->property("text").toString(), QStringLiteral("alice"));

        // delegates of a ListView
        const auto delegate = _engine->findAll(json(R"({"container":{"id":"spaces","type":"ListView"},"type":"Label","text":"space 1"})"));
        QCOMPARE(delegate.size(), size_t(1));
        QCOMPARE(squishTypeName(objectOf(delegate.front())), QStringLiteral("Label"));

        // without a container the search reaches Quick items as well
        QCOMPARE(_engine->findAll(json(R"({"id":"loginButton","type":"Button"})")).size(), size_t(1));
        // only the items of the QML file, not those of some other object tree
        QCOMPARE(_engine->findAll(json(R"({"container":{"name":"window","type":"QWidget"},"type":"Button"})")).size(), size_t(2));
    }

    void containerAndWindowWorkUpwards()
    {
        QuickFixture fixture;
        QVERIFY(fixture.loaded());
        fixture.show();
        QQuickItem *button = fixture.item("loginButton");
        QVERIFY(_engine->matches(button, json(R"({"container":{"name":"quickWidget","type":"QQuickWidget"},"type":"Button"})")));
        QVERIFY(_engine->matches(button, json(R"({"container":{"id":"scrollView","type":"ScrollView"},"type":"Button"})")));
        QVERIFY(!_engine->matches(button, json(R"({"container":{"id":"userNameField","type":"TextField"},"type":"Button"})")));
        QVERIFY(_engine->matches(button, json(R"({"window":{"name":"window","type":"QWidget"},"type":"Button"})")));
        const auto inWindow = _engine->findAll(json(R"({"window":{"name":"window","type":"QWidget"},"id":"loginButton","type":"Button"})"));
        QCOMPARE(inWindow.size(), size_t(1));
        QCOMPARE(objectOf(inWindow.front()), button);
    }

    void overTheProtocol()
    {
        QuickFixture fixture;
        QVERIFY(fixture.loaded());
        fixture.show();
        Probe probe;

        const QJsonObject userName = find(probe, json(userNameSelector));
        QVERIFY(!isNull(userName));
        QCOMPARE(userName.value(QStringLiteral("$class")).toString(), QStringLiteral("TextField"));
        QCOMPARE(get(probe, userName, "id").toString(), QStringLiteral("userNameField"));

        const QJsonObject label = find(probe, json(labelSelector));
        QVERIFY(!isNull(label));
        QCOMPARE(get(probe, label, "text").toString(), QStringLiteral("alice"));
        QCOMPARE(get(probe, label, "id").toString(), QString()); // no id

        const QJsonObject button = find(probe, json(loginButtonSelector));
        QCOMPARE(button.value(QStringLiteral("$class")).toString(), QStringLiteral("Button"));
        QCOMPARE(get(probe, button, "text").toString(), QStringLiteral("Log in"));
        QCOMPARE(get(probe, button, "clicks").toInt(-1), 0);

        const QJsonObject scrollView = find(probe, json(scrollViewSelector));
        QCOMPARE(scrollView.value(QStringLiteral("$class")).toString(), QStringLiteral("ScrollView"));
        QCOMPARE(find(probe, json(labelInScrollViewSelector)), label);

        // the same item keeps its handle
        QCOMPARE(find(probe, json(userNameSelector)), userName);
    }

    void childrenFollowTheItemTree()
    {
        QuickFixture fixture;
        QVERIFY(fixture.loaded());
        fixture.show();
        Probe probe;

        const QJsonObject quickWidget = find(probe, json(quickWidgetSelector));
        const QJsonArray widgetChildren = childrenOf(probe, quickWidget);
        QVERIFY(!widgetChildren.isEmpty());
        const QJsonObject root = widgetChildren.first().toObject();
        QCOMPARE(root.value(QStringLiteral("$class")).toString(), QStringLiteral("Item"));
        QCOMPARE(get(probe, root, "id").toString(), QStringLiteral("page"));

        const QJsonArray rootChildren = childrenOf(probe, root);
        QCOMPARE(rootChildren.size(), 1);
        QCOMPARE(rootChildren.first().toObject(), find(probe, json(scrollViewSelector)));

        // children are exactly the child items
        QQuickItem *scrollView = fixture.item("scrollView");
        const QJsonArray scrollChildren = childrenOf(probe, rootChildren.first().toObject());
        QCOMPARE(scrollChildren.size(), scrollView->childItems().size());
        for (qsizetype i = 0; i < scrollChildren.size(); ++i) {
            QCOMPARE(scrollChildren.at(i).toObject(), probe.registry().ref(scrollView->childItems().at(i)));
        }
    }

    void visibleCombinesTheItemAndItsQuickWidget()
    {
        QuickFixture fixture;
        QVERIFY(fixture.loaded());
        fixture.show();
        Probe probe;

        const QJsonObject label = find(probe, json(labelSelector));
        QCOMPARE(get(probe, label, "visible"), QJsonValue(true));
        // an invisible item
        QVERIFY(isNull(find(probe, json(R"({"id":"hiddenText","type":"Text","visible":true})"))));
        const QJsonObject hidden = find(probe, json(R"({"id":"hiddenText","type":"Text"})"));
        QVERIFY(!isNull(hidden));
        QCOMPARE(get(probe, hidden, "visible"), QJsonValue(false));
        QVERIFY(isNull(find(probe, json(R"({"id":"hiddenText","type":"Text"})"), true)));

        // a hidden QQuickWidget hides its items, though QQuickItem::isVisible() stays true
        fixture.quickWidget->hide();
        QVERIFY(fixture.item("loginButton")->isVisible());
        QCOMPARE(get(probe, label, "visible"), QJsonValue(false));
        QVERIFY(isNull(find(probe, json(R"({"container":{"name":"quickWidget","type":"QQuickWidget"},"type":"Label","visible":true})"))));
        QVERIFY(!isNull(find(probe, json(R"({"container":{"name":"quickWidget","type":"QQuickWidget"},"type":"Label","visible":false})"))));
        QVERIFY(isNull(find(probe, json(R"({"id":"loginButton","type":"Button"})"), true)));
    }

    void readiness()
    {
        QuickFixture fixture;
        QVERIFY(fixture.loaded());
        fixture.show();
        Probe probe;
        QVERIFY(!isNull(find(probe, json(loginButtonSelector), true)));
        QVERIFY(isNull(find(probe, json(R"({"id":"disabledButton","type":"Button"})"), true)));
        QVERIFY(!isNull(find(probe, json(R"({"id":"disabledButton","type":"Button"})"), false)));

        fixture.quickWidget->setEnabled(false);
        QVERIFY(fixture.item("loginButton")->isEnabled());
        QVERIFY(isNull(find(probe, json(loginButtonSelector), true)));
    }

    void itemRectIsInQuickWidgetCoordinates()
    {
        QuickFixture fixture;
        QVERIFY(fixture.loaded());
        fixture.show();
        QQuickItem *button = fixture.item("loginButton");
        QCOMPARE(quickHost(button), fixture.quickWidget);
        QCOMPARE(itemWidget(button), fixture.quickWidget);
        const QRect rect = itemRect(button);
        QVERIFY(rect.isValid());
        QCOMPARE(rect, button->mapRectToScene(button->boundingRect()).toAlignedRect());
        QCOMPARE(quickItemRect(button), rect);
        // the button sits below the text field, inside the widget
        QVERIFY(rect.top() >= itemRect(fixture.item("userNameField")).bottom());
        QVERIFY(fixture.quickWidget->rect().contains(rect));

        // an item outside any QQuickWidget has no rect
        QQuickItem loose;
        QCOMPARE(quickHost(&loose), nullptr);
        QCOMPARE(itemWidget(&loose), nullptr);
        QVERIFY(!itemRect(&loose).isValid());
    }

    void propertyPrecedence()
    {
        // a real property wins over hooks, hooks win over the type readers (here the Quick `id`), for `get` and
        // selector keys alike
        QuickFixture fixture;
        QVERIFY(fixture.loaded());
        fixture.show();
        Probe probe;
        probe.registerPropertyHook([](const Target &target, const QString &name) -> std::optional<QJsonValue> {
            if (!qobject_cast<QQuickItem *>(objectOf(target))) {
                return std::nullopt;
            }
            if (name == QLatin1String("id") || name == QLatin1String("text")) {
                return QStringLiteral("fromHook");
            }
            return std::nullopt;
        });
        const QJsonObject button = find(probe, json(R"({"type":"Button","text":"Log in"})"));
        QVERIFY(!isNull(button));
        QCOMPARE(get(probe, button, "text").toString(), QStringLiteral("Log in"));
        QCOMPARE(get(probe, button, "id").toString(), QStringLiteral("fromHook"));
        QCOMPARE(findAll(probe, json(R"({"type":"Button","id":"loginButton"})")).size(), 0);
        QCOMPARE(find(probe, json(R"({"type":"Button","id":"fromHook","text":"Log in"})")), button);
    }

    void hooksWinOverItemReaders()
    {
        QTabBar bar;
        bar.addTab(QStringLiteral("Local Activity"));
        Probe probe;
        probe.registerPropertyHook([](const Target &target, const QString &name) -> std::optional<QJsonValue> {
            if (std::holds_alternative<SyntheticItem>(target) && name == QLatin1String("text")) {
                return QStringLiteral("fromHook");
            }
            return std::nullopt;
        });
        bar.setObjectName(QStringLiteral("bar"));
        const QJsonObject tab = find(probe, json(R"({"type":"TabItem","container":{"name":"bar","type":"QTabBar"},"index":0})"));
        QVERIFY(!isNull(tab));
        QCOMPARE(get(probe, tab, "text").toString(), QStringLiteral("fromHook"));
        QCOMPARE(find(probe, json(R"({"type":"TabItem","container":{"name":"bar","type":"QTabBar"},"text":"fromHook"})")), tab);
        QVERIFY(isNull(find(probe, json(R"({"type":"TabItem","container":{"name":"bar","type":"QTabBar"},"text":"Local Activity"})"))));
    }
};

QTEST_MAIN(TestQuick)
#include "tst_quick.moc"
