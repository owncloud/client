// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#include "probe.h"
#include "registry.h"
#include "selector.h"

#include <QAction>
#include <QCheckBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QSlider>
#include <QStackedWidget>
#include <QTest>
#include <QVBoxLayout>
#include <QWizard>

#include <memory>

using namespace QtGuiProbe;

namespace Fixture {
class TestDialog : public QDialog
{
    Q_OBJECT
public:
    using QDialog::QDialog;
};

class EmbeddedWizard : public QWizard
{
    Q_OBJECT
public:
    using QWizard::QWizard;
};
} // namespace Fixture

namespace {

QJsonObject json(const char *text)
{
    return QJsonDocument::fromJson(QByteArray(text)).object();
}

QJsonObject pattern(const char *kind, const char *value)
{
    return {{QString::fromLatin1(kind), QString::fromLatin1(value)}};
}

QObject *objectOf(const std::optional<Target> &target)
{
    if (!target) {
        return nullptr;
    }
    const auto *object = std::get_if<QObject *>(&*target);
    return object ? *object : nullptr;
}

QObjectList objectsOf(const std::vector<Target> &targets)
{
    QObjectList objects;
    for (const auto &target : targets) {
        const auto *object = std::get_if<QObject *>(&target);
        objects.append(object ? *object : nullptr);
    }
    return objects;
}

QJsonObject request(Probe &probe, const QString &method, const QJsonObject &params = {})
{
    return probe.handle(QJsonObject{{QStringLiteral("id"), 1}, {QStringLiteral("method"), method}, {QStringLiteral("params"), params}});
}

QString errorCode(const QJsonObject &response)
{
    return response.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString();
}

const char settingsSelector[] = R"({"name":"Settings","type":"QDialog","visible":1})";
const char stackSelector[] = R"({"name":"stack","type":"QStackedWidget","visible":1,"window":{"name":"Settings","type":"QDialog","visible":1}})";
const char checkBoxSelector[] = R"({"name":"check","type":"QCheckBox","visible":1,"container":{"name":"stack","type":"QStackedWidget","visible":1,
    "window":{"name":"Settings","type":"QDialog","visible":1}}})";

// A QDialog "Settings" holding a QStackedWidget "stack", whose page holds an unnamed label, a checkbox and
// another unnamed label, top to bottom.
struct SettingsFixture
{
    SettingsFixture()
    {
        dialog.setObjectName(QStringLiteral("Settings"));
        auto *layout = new QVBoxLayout(&dialog);
        stack = new QStackedWidget(&dialog);
        stack->setObjectName(QStringLiteral("stack"));
        layout->addWidget(stack);
        auto *page = new QWidget;
        auto *pageLayout = new QVBoxLayout(page);
        upper = new QLabel(QStringLiteral("Status"), page);
        check = new QCheckBox(QStringLiteral("Enable"), page);
        check->setObjectName(QStringLiteral("check"));
        lower = new QLabel(QStringLiteral("Errors: 12"), page);
        pageLayout->addWidget(upper);
        pageLayout->addWidget(check);
        pageLayout->addWidget(lower);
        stack->addWidget(page);
        dialog.show();
    }

    bool exposed() { return QTest::qWaitForWindowExposed(&dialog); }

    QDialog dialog;
    QStackedWidget *stack;
    QLabel *upper;
    QCheckBox *check;
    QLabel *lower;
};

} // namespace

class TestSelector : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void findsByNameTypeAndWindow()
    {
        SettingsFixture fixture;
        QVERIFY(fixture.exposed());
        Registry registry;
        SelectorEngine engine(registry);
        QCOMPARE(objectsOf(engine.findAll(json(stackSelector))), QObjectList({fixture.stack}));
        QCOMPARE(objectOf(engine.find(json(stackSelector), true)), fixture.stack);
        QCOMPARE(objectOf(engine.find(json(settingsSelector), true)), &fixture.dialog);
    }

    void unnamedOccurrenceInContainer()
    {
        SettingsFixture fixture;
        QVERIFY(fixture.exposed());
        Registry registry;
        SelectorEngine engine(registry);
        const QJsonObject stack = json(stackSelector);
        QJsonObject selector{{QStringLiteral("type"), QStringLiteral("QLabel")}, {QStringLiteral("unnamed"), 1}, {QStringLiteral("container"), stack}};
        QCOMPARE(objectsOf(engine.findAll(selector)), QObjectList({fixture.upper, fixture.lower}));
        selector.insert(QStringLiteral("occurrence"), 2);
        QCOMPARE(objectsOf(engine.findAll(selector)), QObjectList({fixture.lower}));
        QCOMPARE(objectOf(engine.find(selector, true)), fixture.lower);
        selector.insert(QStringLiteral("occurrence"), 3);
        QVERIFY(!engine.find(selector, false));
        // a named label is not unnamed
        fixture.upper->setObjectName(QStringLiteral("upper"));
        selector.insert(QStringLiteral("occurrence"), 1);
        QCOMPARE(objectOf(engine.find(selector, false)), fixture.lower);
    }

    void regexText()
    {
        SettingsFixture fixture;
        QVERIFY(fixture.exposed());
        Registry registry;
        SelectorEngine engine(registry);
        QCOMPARE(objectsOf(engine.findAll(json(R"({"text":{"$regex":"^Errors: [0-9]+$"},"type":"QLabel"})"))), QObjectList({fixture.lower}));
        QCOMPARE(objectsOf(engine.findAll(json(R"({"text":{"$wildcard":"Err*"},"type":"QLabel"})"))), QObjectList({fixture.lower}));
        QCOMPARE(objectsOf(engine.findAll(json(R"({"text":"Status","type":"QLabel"})"))), QObjectList({fixture.upper}));
    }

    void aboveWidgetIsTheWidgetAbove()
    {
        // Squish semantics: "aboveWidget: S" names the widget above the one searched for (the suite's
        // selective-sync tree sits below its label), so the label below the checkbox matches.
        SettingsFixture fixture;
        QVERIFY(fixture.exposed());
        Registry registry;
        SelectorEngine engine(registry);
        QJsonObject selector{
            {QStringLiteral("type"), QStringLiteral("QLabel")}, {QStringLiteral("unnamed"), 1}, {QStringLiteral("aboveWidget"), json(checkBoxSelector)}};
        QCOMPARE(objectsOf(engine.findAll(selector)), QObjectList({fixture.lower}));
        // the nearest one wins
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"QLabel","aboveWidget":{"text":"Status","type":"QLabel"}})"))), QObjectList({fixture.lower}));
        QJsonObject nearest{{QStringLiteral("visible"), 1}, {QStringLiteral("container"), json(stackSelector)},
            {QStringLiteral("aboveWidget"), json(R"({"text":"Status","type":"QLabel"})")}};
        QCOMPARE(objectsOf(engine.findAll(nearest)), QObjectList({fixture.check}));
        // no match when the widget named is not found
        QVERIFY(engine.findAll(json(R"({"type":"QLabel","aboveWidget":{"name":"missing"}})")).empty());
    }

    void leftWidgetIsTheWidgetToTheLeft()
    {
        QDialog dialog;
        auto *rows = new QVBoxLayout(&dialog);
        auto *row = new QHBoxLayout;
        rows->addLayout(row);
        auto *edit = new QLineEdit(&dialog);
        edit->setObjectName(QStringLiteral("edit"));
        auto *near = new QPushButton(QStringLiteral("near"), &dialog);
        auto *far = new QPushButton(QStringLiteral("far"), &dialog);
        row->addWidget(edit);
        row->addWidget(near);
        row->addWidget(far);
        auto *below = new QPushButton(QStringLiteral("below"), &dialog); // right of nothing on its row
        rows->addWidget(below);
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        Registry registry;
        SelectorEngine engine(registry);
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"QPushButton","leftWidget":{"name":"edit","type":"QLineEdit"}})"))), QObjectList({near}));
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"QPushButton","leftWidget":{"text":"near","type":"QPushButton"}})"))), QObjectList({far}));
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"QPushButton","aboveWidget":{"name":"edit","type":"QLineEdit"}})"))), QObjectList({below}));
    }

    void valueMatchesComparesByValue()
    {
        QVERIFY(valueMatches(QJsonValue(QStringLiteral("0")), QVariant(0)));
        QVERIFY(valueMatches(QJsonValue(1), QVariant(true)));
        QVERIFY(valueMatches(QJsonValue(true), QVariant(QStringLiteral("true"))));
        QVERIFY(valueMatches(QJsonValue(0), QVariant(false)));
        QVERIFY(valueMatches(QJsonValue(false), QVariant(false)));
        QVERIFY(valueMatches(QJsonValue(2), QVariant(2.0)));
        QVERIFY(valueMatches(QJsonValue(QStringLiteral("abc")), QVariant(QStringLiteral("abc"))));
        QVERIFY(!valueMatches(QJsonValue(QStringLiteral("1")), QVariant(0)));
        QVERIFY(!valueMatches(QJsonValue(1), QVariant(false)));
        QVERIFY(!valueMatches(QJsonValue(false), QVariant(QStringLiteral("true"))));
        QVERIFY(!valueMatches(QJsonValue(QStringLiteral("abc")), QVariant(QStringLiteral("abcd"))));
        QVERIFY(!valueMatches(QJsonValue(QStringLiteral("x")), QVariant(0)));
        // two strings compare as strings, even when both look numeric
        QVERIFY(!valueMatches(QJsonValue(QStringLiteral("1.0")), QVariant(QStringLiteral("1"))));
        QVERIFY(valueMatches(pattern("$regex", "b+"), QVariant(QStringLiteral("abbc"))));
        QVERIFY(!valueMatches(pattern("$regex", "^b"), QVariant(QStringLiteral("abbc"))));
        QVERIFY(valueMatches(pattern("$wildcard", "a*/c?"), QVariant(QStringLiteral("ab/cd"))));
        QVERIFY(!valueMatches(pattern("$wildcard", "a*"), QVariant(QStringLiteral("ba"))));
        QVERIFY_THROWS_EXCEPTION(ProbeError, valueMatches(pattern("$regex", "("), QVariant(QStringLiteral("x"))));
        QVERIFY_THROWS_EXCEPTION(ProbeError, valueMatches(pattern("$nope", "x"), QVariant(QStringLiteral("x"))));
    }

    void enumsCompareByKeyOrValue()
    {
        QDialog dialog;
        auto *layout = new QVBoxLayout(&dialog);
        auto *horizontal = new QSlider(Qt::Horizontal, &dialog);
        auto *vertical = new QSlider(Qt::Vertical, &dialog);
        layout->addWidget(horizontal);
        layout->addWidget(vertical);
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        Registry registry;
        SelectorEngine engine(registry);
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"QSlider","orientation":1})"))), QObjectList({horizontal}));
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"QSlider","orientation":"Vertical"})"))), QObjectList({vertical}));
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"QSlider","orientation":"Qt::Vertical"})"))), QObjectList({vertical}));
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"QSlider","orientation":"2"})"))), QObjectList({vertical}));
        // bool properties and unknown properties
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"QSlider","tracking":true,"orientation":1})"))), QObjectList({horizontal}));
        QVERIFY(engine.findAll(json(R"({"type":"QSlider","noSuchProperty":1})")).empty());
    }

    void readyRequiresVisibleAndEnabled()
    {
        SettingsFixture fixture;
        QVERIFY(fixture.exposed());
        Registry registry;
        SelectorEngine engine(registry);
        const QJsonObject selector = json(R"({"name":"check","type":"QCheckBox"})");
        QCOMPARE(objectOf(engine.find(selector, true)), fixture.check);
        fixture.check->setEnabled(false);
        QVERIFY(!engine.find(selector, true));
        QCOMPARE(objectOf(engine.find(selector, false)), fixture.check);
        fixture.check->setEnabled(true);
        fixture.check->hide();
        QVERIFY(!engine.find(selector, true));
        QCOMPARE(objectOf(engine.find(selector, false)), fixture.check);
        // "visible": 1 in the selector skips the hidden one altogether
        QVERIFY(!engine.find(json(R"({"name":"check","type":"QCheckBox","visible":1})"), false));
    }

    void namespacedClass()
    {
        Fixture::TestDialog dialog;
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        Registry registry;
        SelectorEngine engine(registry);
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"Fixture::TestDialog"})"))), QObjectList({&dialog}));
        // the exact type, not a base class
        QVERIFY(!objectsOf(engine.findAll(json(R"({"type":"QDialog"})"))).contains(&dialog));
    }

    void topLevelWindowsInShowOrder()
    {
        Fixture::TestDialog second;
        Fixture::TestDialog first;
        first.show();
        QVERIFY(QTest::qWaitForWindowExposed(&first));
        second.show();
        QVERIFY(QTest::qWaitForWindowExposed(&second));
        Registry registry;
        SelectorEngine engine(registry);
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"Fixture::TestDialog","visible":1})"))), QObjectList({&first, &second}));
        QCOMPARE(objectOf(engine.find(json(R"({"type":"Fixture::TestDialog","visible":1,"occurrence":2})"), true)), &second);
    }

    // A dialog embedded in another window (flags reset to Qt::Widget, put into a layout) is not a window to Qt, but
    // Squish still names its children by it: `window` accepts any dialog or main window among the ancestors.
    void windowMatchesEmbeddedDialogs()
    {
        QMainWindow mainWindow;
        mainWindow.setObjectName(QStringLiteral("MainWindow"));
        auto *stack = new QStackedWidget(&mainWindow);
        mainWindow.setCentralWidget(stack);
        auto *page = new QWidget;
        auto *pageLayout = new QVBoxLayout(page);
        auto *panel = new QWidget(page);
        panel->setObjectName(QStringLiteral("Panel"));
        auto *panelLayout = new QVBoxLayout(panel);
        auto *outside = new QPushButton(QStringLiteral("Outside"), panel);
        outside->setObjectName(QStringLiteral("outside"));
        panelLayout->addWidget(outside);
        auto *wizard = new Fixture::EmbeddedWizard(page);
        wizard->setObjectName(QStringLiteral("AddFolderSyncWizard"));
        wizard->setWindowFlags(Qt::Widget);
        wizard->addPage(new QWizardPage);
        wizard->addPage(new QWizardPage);
        pageLayout->addWidget(panel);
        pageLayout->addWidget(wizard);
        stack->addWidget(page);
        mainWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&mainWindow));
        QVERIFY(!wizard->isWindow());
        QVERIFY(wizard->isVisible());
        QAbstractButton *cancel = wizard->button(QWizard::CancelButton);
        QAbstractButton *next = wizard->button(QWizard::NextButton);
        QCOMPARE(cancel->objectName(), QStringLiteral("qt_wizard_cancel"));
        QVERIFY(cancel->isVisible());
        Registry registry;
        SelectorEngine engine(registry);

        const QJsonObject cancelInWizard = json(R"({"window":{"name":"AddFolderSyncWizard","type":"Fixture::EmbeddedWizard","visible":1},
            "name":"qt_wizard_cancel","type":"QPushButton","visible":1})");
        QCOMPARE(objectsOf(engine.findAll(cancelInWizard)), QObjectList({cancel}));
        QCOMPARE(objectOf(engine.find(cancelInWizard, true)), cancel);
        const QJsonObject nextInWizard = json(R"({"window":{"name":"AddFolderSyncWizard","type":"Fixture::EmbeddedWizard","visible":1},
            "name":"__qt__passive_wizardbutton1","type":"QPushButton","visible":1})");
        QCOMPARE(objectsOf(engine.findAll(nextInWizard)), QObjectList({next}));
        // the true top-level window still counts
        QCOMPARE(objectsOf(engine.findAll(json(R"({"window":{"name":"MainWindow","type":"QMainWindow","visible":1},
            "name":"qt_wizard_cancel","type":"QPushButton","visible":1})"))),
            QObjectList({cancel}));
        // a handle of the embedded dialog works as well, with and without a scope search
        const QJsonObject wizardRef = registry.ref(wizard);
        QCOMPARE(objectsOf(engine.findAll({{QStringLiteral("window"), wizardRef}, {QStringLiteral("name"), QStringLiteral("qt_wizard_cancel")}})),
            QObjectList({cancel}));
        QVERIFY(engine.matches(cancel, {{QStringLiteral("window"), wizardRef}}));
        // a widget outside the dialog does not belong to it
        QVERIFY(!engine.matches(outside, {{QStringLiteral("window"), wizardRef}}));
        const QJsonObject outsideInWizard =
            json(R"({"window":{"name":"AddFolderSyncWizard","type":"Fixture::EmbeddedWizard","visible":1},"name":"outside","type":"QPushButton"})");
        QVERIFY(engine.findAll(outsideInWizard).empty());
        // a plain container is no window
        QVERIFY(engine.findAll(json(R"({"window":{"name":"Panel","type":"QWidget","visible":1},"name":"outside","type":"QPushButton"})")).empty());
        QVERIFY(!engine.matches(outside, {{QStringLiteral("window"), registry.ref(panel)}}));
    }

    void menuActionsAreSearched()
    {
        QDialog window;
        window.setObjectName(QStringLiteral("main"));
        QObject elsewhere;
        auto *menu = new QMenu(&window);
        menu->setObjectName(QStringLiteral("menu"));
        auto *foreign = new QAction(QStringLiteral("Remove"), &elsewhere); // its QObject parent is not the menu
        menu->addAction(foreign);
        menu->addAction(QStringLiteral("Keep"));
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        menu->popup(window.mapToGlobal(QPoint(0, 0)));
        QVERIFY(QTest::qWaitForWindowExposed(menu));
        Registry registry;
        SelectorEngine engine(registry);
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"QAction","text":"Remove","container":{"name":"menu","type":"QMenu","visible":1}})"))),
            QObjectList({foreign}));
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"QAction","text":"Remove","window":{"name":"menu","type":"QMenu"}})"))), QObjectList({foreign}));
        QCOMPARE(objectOf(engine.find(json(R"({"type":"QAction","text":"Remove","container":{"name":"menu","type":"QMenu"}})"), true)), foreign);
        // a popup menu belongs to the window it pops up from
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"QMenu","visible":1,"window":{"name":"main","type":"QDialog"}})"))), QObjectList({menu}));
        QVERIFY(engine.matches(foreign, json(R"({"type":"QAction","container":{"name":"menu"}})")));
        menu->hide();
    }

    void buddyAndHandleValues()
    {
        QDialog dialog;
        auto *layout = new QVBoxLayout(&dialog);
        auto *label = new QLabel(QStringLiteral("File name:"), &dialog);
        label->setObjectName(QStringLiteral("fileNameLabel"));
        auto *edit = new QLineEdit(&dialog);
        edit->setObjectName(QStringLiteral("fileNameEdit"));
        auto *other = new QLineEdit(&dialog);
        label->setBuddy(edit);
        layout->addWidget(label);
        layout->addWidget(edit);
        layout->addWidget(other);
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        Registry registry;
        SelectorEngine engine(registry);
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"QLineEdit","buddy":{"name":"fileNameLabel","type":"QLabel"}})"))), QObjectList({edit}));
        // a handle as container, window and property value
        QJsonObject selector{{QStringLiteral("type"), QStringLiteral("QLineEdit")}, {QStringLiteral("container"), registry.ref(&dialog)}};
        QCOMPARE(objectsOf(engine.findAll(selector)), QObjectList({edit, other}));
        selector.remove(QStringLiteral("container"));
        selector.insert(QStringLiteral("window"), registry.ref(&dialog));
        QCOMPARE(objectsOf(engine.findAll(selector)), QObjectList({edit, other}));
        selector.insert(QStringLiteral("buddy"), registry.ref(label));
        QCOMPARE(objectsOf(engine.findAll(selector)), QObjectList({edit}));
        // a selector that is a handle stands for that object
        QCOMPARE(objectOf(engine.find(registry.ref(other), false)), other);
    }

    void typeResolversSearchScopes()
    {
        SettingsFixture fixture;
        QVERIFY(fixture.exposed());
        Registry registry;
        SelectorEngine engine(registry);
        std::vector<Target> seenScopes;
        engine.addTypeResolver([&](const QJsonObject &sel, const Target &scope) -> std::optional<std::vector<Target>> {
            if (sel.value(QStringLiteral("type")).toString() != QStringLiteral("Item")) {
                return std::nullopt;
            }
            seenScopes.push_back(scope);
            QObject *owner = std::get<QObject *>(scope);
            if (!owner) {
                return std::vector<Target>{};
            }
            return std::vector<Target>{SyntheticItem{owner, QStringLiteral("Item"), 0}, SyntheticItem{owner, QStringLiteral("Item"), 1}};
        });
        // one scope per container match, merged in scope order; occurrence counts across scopes
        QJsonObject selector{{QStringLiteral("type"), QStringLiteral("Item")}, {QStringLiteral("container"), json(R"({"type":"QLabel","unnamed":1})")}};
        std::vector<Target> items = engine.findAll(selector);
        QCOMPARE(items.size(), size_t(4));
        QCOMPARE(seenScopes.size(), size_t(2));
        QCOMPARE(std::get<QObject *>(seenScopes[0]), fixture.upper);
        QCOMPARE(std::get<QObject *>(seenScopes[1]), fixture.lower);
        selector.insert(QStringLiteral("occurrence"), 3);
        items = engine.findAll(selector);
        QCOMPARE(items.size(), size_t(1));
        QCOMPARE(std::get<SyntheticItem>(items[0]).owner.data(), fixture.lower);
        QCOMPARE(std::get<SyntheticItem>(items[0]).index, 0);
        // a ready item needs a visible owner
        QVERIFY(engine.find(selector, true));
        fixture.lower->hide();
        QVERIFY(!engine.find(selector, true));
        // without a container the resolver gets the whole application as a null scope
        seenScopes.clear();
        QVERIFY(engine.findAll(json(R"({"type":"Item"})")).empty());
        QCOMPARE(seenScopes.size(), size_t(1));
        QCOMPARE(std::get<QObject *>(seenScopes[0]), nullptr);
        // a declined selector falls back to the widget search
        QCOMPARE(objectsOf(engine.findAll(json(R"({"type":"QCheckBox","name":"check"})"))), QObjectList({fixture.check}));
    }

    void propertyReadersAndReadyChecks()
    {
        SettingsFixture fixture;
        QVERIFY(fixture.exposed());
        Registry registry;
        SelectorEngine engine(registry);
        engine.addPropertyReader([&](const Target &target, const QString &name) -> std::optional<QJsonValue> {
            if (name == QStringLiteral("pseudo") && std::get<QObject *>(target) == fixture.check) {
                return QJsonValue(5);
            }
            return std::nullopt;
        });
        QCOMPARE(objectsOf(engine.findAll(json(R"({"pseudo":"5"})"))), QObjectList({fixture.check}));
        // real properties win over readers
        engine.addPropertyReader([](const Target &, const QString &name) -> std::optional<QJsonValue> {
            return name == QStringLiteral("text") ? std::optional<QJsonValue>(QStringLiteral("fake")) : std::nullopt;
        });
        QVERIFY(engine.findAll(json(R"({"type":"QLabel","text":"fake"})")).empty());
        engine.addReadyCheck([&](const Target &target) -> std::optional<bool> {
            if (std::get<QObject *>(target) == fixture.check) {
                return false;
            }
            return std::nullopt;
        });
        QVERIFY(!engine.find(json(R"({"name":"check"})"), true));
        QVERIFY(engine.find(json(R"({"name":"stack"})"), true));
    }

    void probeServesFindAndFindAll()
    {
        SettingsFixture fixture;
        QVERIFY(fixture.exposed());
        Probe probe;
        QJsonObject response = request(probe, QStringLiteral("find"), {{QStringLiteral("selector"), json(stackSelector)}, {QStringLiteral("ready"), true}});
        QCOMPARE(response.value(QStringLiteral("result")).toObject(), probe.registry().ref(fixture.stack));
        QCOMPARE(response.value(QStringLiteral("result")).toObject().value(QStringLiteral("$class")).toString(), QStringLiteral("QStackedWidget"));
        // ready is optional
        response = request(probe, QStringLiteral("find"), {{QStringLiteral("selector"), json(R"({"name":"nothing"})")}});
        QCOMPARE(response.value(QStringLiteral("result")).toObject(), Registry::nullRef());
        response = request(probe, QStringLiteral("findAll"), {{QStringLiteral("selector"), json(R"({"type":"QLabel","unnamed":1,"visible":1})")}});
        QCOMPARE(response.value(QStringLiteral("result")).toArray(), QJsonArray({probe.registry().ref(fixture.upper), probe.registry().ref(fixture.lower)}));
        // property hooks of the probe serve selector keys, too
        probe.registerPropertyHook([&](const Target &target, const QString &name) -> std::optional<QJsonValue> {
            if (name == QStringLiteral("pseudo") && std::get<QObject *>(target) == fixture.lower) {
                return QJsonValue(true);
            }
            return std::nullopt;
        });
        response = request(probe, QStringLiteral("findAll"), {{QStringLiteral("selector"), json(R"({"pseudo":1})")}});
        QCOMPARE(response.value(QStringLiteral("result")).toArray(), QJsonArray({probe.registry().ref(fixture.lower)}));
        // engine extensions are reachable through the probe
        probe.selectors().addReadyCheck([&](const Target &target) -> std::optional<bool> {
            return std::get<QObject *>(target) == fixture.stack ? std::optional<bool>(false) : std::nullopt;
        });
        response = request(probe, QStringLiteral("find"), {{QStringLiteral("selector"), json(stackSelector)}, {QStringLiteral("ready"), true}});
        QCOMPARE(response.value(QStringLiteral("result")).toObject(), Registry::nullRef());
    }

    void malformedSelectorsAreBadRequests()
    {
        SettingsFixture fixture;
        QVERIFY(fixture.exposed());
        Probe probe;
        const auto findCode = [&probe](const QJsonObject &params) { return errorCode(request(probe, QStringLiteral("find"), params)); };
        QCOMPARE(findCode({}), QStringLiteral("BadRequest"));
        QCOMPARE(findCode({{QStringLiteral("selector"), QStringLiteral("stack")}}), QStringLiteral("BadRequest"));
        QCOMPARE(findCode({{QStringLiteral("selector"), json(stackSelector)}, {QStringLiteral("ready"), QStringLiteral("yes")}}), QStringLiteral("BadRequest"));
        QCOMPARE(findCode({{QStringLiteral("selector"), json(R"({"type":"QLabel","occurrence":0})")}}), QStringLiteral("BadRequest"));
        QCOMPARE(findCode({{QStringLiteral("selector"), json(R"({"type":"QLabel","occurrence":"x"})")}}), QStringLiteral("BadRequest"));
        QCOMPARE(findCode({{QStringLiteral("selector"), json(R"({"type":"QLabel","container":"stack"})")}}), QStringLiteral("BadRequest"));
        QCOMPARE(findCode({{QStringLiteral("selector"),
                     QJsonObject{{QStringLiteral("type"), QStringLiteral("QLabel")}, {QStringLiteral("text"), pattern("$regex", "(")}}}}),
            QStringLiteral("BadRequest"));
        QCOMPARE(findCode({{QStringLiteral("selector"), json(R"({"type":"QLabel","container":{"$handle":999}})")}}), QStringLiteral("NoSuchObject"));
        QCOMPARE(errorCode(request(probe, QStringLiteral("findAll"), {})), QStringLiteral("BadRequest"));
        // a numeric occurrence string is accepted, as Squish does
        const QJsonObject ok =
            request(probe, QStringLiteral("find"), {{QStringLiteral("selector"), json(R"({"type":"QLabel","unnamed":1,"occurrence":"2"})")}});
        QCOMPARE(ok.value(QStringLiteral("result")).toObject(), probe.registry().ref(fixture.lower));
    }
};

QTEST_MAIN(TestSelector)
#include "tst_selector.moc"
