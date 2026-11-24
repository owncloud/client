// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#include "actions.h"
#include "keys.h"
#include "probe.h"
#include "registry.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QElapsedTimer>
#include <QHeaderView>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QQuickItem>
#include <QQuickWidget>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <optional>

using namespace QtGuiProbe;
using namespace Qt::StringLiterals;

namespace {

QJsonObject request(Probe &probe, const QString &method, const QJsonObject &params = {})
{
    return probe.handle(QJsonObject{{u"id"_s, 1}, {u"method"_s, method}, {u"params"_s, params}});
}

QJsonValue result(const QJsonObject &response)
{
    if (response.contains(u"error"_s)) {
        qWarning() << "error response" << response;
    }
    return response.value(u"result"_s);
}

QString errorCode(const QJsonObject &response)
{
    return response.value(u"error"_s).toObject().value(u"code"_s).toString();
}

// Only the handle of a reference, as the client sends it back.
QJsonObject handleOf(const QJsonValue &ref)
{
    return QJsonObject{{u"$handle"_s, ref.toObject().value(u"$handle"_s)}};
}

QJsonObject refOf(Probe &probe, QObject *object)
{
    return handleOf(probe.registry().ref(object));
}

QJsonObject find(Probe &probe, const QJsonObject &selector)
{
    return result(request(probe, u"find"_s, {{u"selector"_s, selector}, {u"ready"_s, false}})).toObject();
}

QJsonObject click(const QJsonObject &target, int clicks = 1)
{
    return {{u"handle"_s, target}, {u"button"_s, 1}, {u"modifiers"_s, 0}, {u"clicks"_s, clicks}};
}

QJsonObject clickAt(const QJsonObject &target, int x, int y)
{
    QJsonObject params = click(target);
    params.insert(u"x"_s, x);
    params.insert(u"y"_s, y);
    return params;
}

// Lets posted input run.
void runPostedInput()
{
    for (int i = 0; i < 3; ++i) {
        QCoreApplication::processEvents();
    }
}

// A client on Probe::connectTransport: requests go through the same line buffer as on the socket.
class TransportClient
{
public:
    explicit TransportClient(Probe &probe)
        : _probe(probe)
    {
        _probe.connectTransport([this](const QByteArray &line) {
            const QJsonObject response = QJsonDocument::fromJson(line).object();
            _responses.insert(response.value(u"id"_s).toInt(), response);
        });
    }

    static QByteArray line(int id, const QString &method, const QJsonObject &params)
    {
        return QJsonDocument(QJsonObject{{u"id"_s, id}, {u"method"_s, method}, {u"params"_s, params}}).toJson(QJsonDocument::Compact) + '\n';
    }

    int send(const QString &method, const QJsonObject &params = {})
    {
        const int id = _nextId++;
        _probe.receive(line(id, method, params));
        return id;
    }

    int nextId() { return _nextId++; }

    std::optional<QJsonObject> response(int id) const
    {
        const auto it = _responses.constFind(id);
        return it == _responses.cend() ? std::nullopt : std::optional<QJsonObject>(*it);
    }

    qsizetype responseCount() const { return _responses.size(); }

private:
    Probe &_probe;
    QHash<int, QJsonObject> _responses;
    int _nextId = 1;
};

// A tree like the selective sync dialog's: checkable "Personal" (with "folder1") and "Shares".
struct TreeFixture
{
    QWidget window;
    QTreeWidget *tree = new QTreeWidget(&window);
    QTreeWidgetItem *personal = nullptr;
    QTreeWidgetItem *folder1 = nullptr;

    TreeFixture()
    {
        auto *layout = new QVBoxLayout(&window);
        layout->addWidget(tree);
        tree->setHeaderLabels({u"Name"_s});
        personal = new QTreeWidgetItem(tree, {u"Personal"_s});
        personal->setFlags(personal->flags() | Qt::ItemIsUserCheckable);
        personal->setCheckState(0, Qt::Checked);
        folder1 = new QTreeWidgetItem(personal, {u"folder1"_s});
        new QTreeWidgetItem(tree, {u"Shares"_s});
        window.resize(400, 300);
        window.show();
    }

    QJsonObject index(Probe &probe, const QString &text)
    {
        return handleOf(find(probe, {{u"type"_s, u"QModelIndex"_s}, {u"container"_s, refOf(probe, tree)}, {u"text"_s, text}}));
    }
};

} // namespace

class TestActions : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesSquishKeys()
    {
        const auto strokes = parseSquishKeys(u"ab<Ctrl+A><Return>Ä"_s);
        QCOMPARE(strokes.size(), 5u);
        QCOMPARE(strokes[0].key, Qt::Key_A);
        QCOMPARE(strokes[0].mods, Qt::NoModifier);
        QCOMPARE(strokes[0].text, u"a"_s);
        QCOMPARE(strokes[1].key, Qt::Key_B);
        QCOMPARE(strokes[1].text, u"b"_s);
        QCOMPARE(strokes[2].key, Qt::Key_A);
        QCOMPARE(strokes[2].mods, Qt::ControlModifier);
        QCOMPARE(strokes[2].text, QString());
        QCOMPARE(strokes[3].key, Qt::Key_Return);
        QCOMPARE(strokes[3].mods, Qt::NoModifier);
        QCOMPARE(strokes[4].key, Qt::Key_Adiaeresis);
        QCOMPARE(strokes[4].text, u"Ä"_s);
    }

    void parsesTokensCaseInsensitively()
    {
        auto strokes = parseSquishKeys(u"<escape><ESC><shift+tab><alt+f4><F12><ctrl+shift+z><Space><<"_s);
        QCOMPARE(strokes.size(), 8u);
        QCOMPARE(strokes[0].key, Qt::Key_Escape);
        QCOMPARE(strokes[1].key, Qt::Key_Escape);
        QCOMPARE(strokes[2].key, Qt::Key_Backtab);
        QCOMPARE(strokes[2].mods, Qt::ShiftModifier);
        QCOMPARE(strokes[3].key, Qt::Key_F4);
        QCOMPARE(strokes[3].mods, Qt::AltModifier);
        QCOMPARE(strokes[4].key, Qt::Key_F12);
        QCOMPARE(strokes[5].key, Qt::Key_Z);
        QCOMPARE(strokes[5].mods, Qt::ControlModifier | Qt::ShiftModifier);
        QCOMPARE(strokes[6].key, Qt::Key_Space);
        QCOMPARE(strokes[6].text, u" "_s);
        QCOMPARE(strokes[7].key, Qt::Key_Less);
        QCOMPARE(strokes[7].text, u"<"_s);

        // a character outside the BMP is one stroke; a lone '>' types itself
        strokes = parseSquishKeys(u"🙂>"_s);
        QCOMPARE(strokes.size(), 2u);
        QCOMPARE(strokes[0].text, u"🙂"_s);
        QCOMPARE(strokes[1].text, u">"_s);
        QCOMPARE(strokes[1].key, Qt::Key_Greater);
    }

    void rejectsUnknownTokens()
    {
        for (const QString &keys : {u"<Frobnicate>"_s, u"<Ctrl+>"_s, u"<Hyper+A>"_s, u"abc<Return"_s, u"<>"_s}) {
            try {
                parseSquishKeys(keys);
                QFAIL(qPrintable(u"no error for "_s + keys));
            } catch (const ProbeError &error) {
                QCOMPARE(error.code, u"BadRequest"_s);
            }
        }
    }

    void typesUnicodeText()
    {
        QWidget window;
        auto *edit = new QLineEdit(&window);
        window.show();
        Probe probe;
        const QJsonObject response = request(probe, u"typeText"_s, {{u"handle"_s, refOf(probe, edit)}, {u"text"_s, u"Grüße 🙂"_s}});
        QCOMPARE(result(response), QJsonValue());
        QTRY_COMPARE(edit->text(), u"Grüße 🙂"_s);
    }

    void typesKeyTokens()
    {
        QWidget window;
        auto *edit = new QLineEdit(u"abc"_s, &window);
        window.show();
        Probe probe;
        request(probe, u"typeText"_s, {{u"handle"_s, refOf(probe, edit)}, {u"text"_s, u"x<Ctrl+A><Backspace>"_s}});
        QTRY_VERIFY(edit->text().isEmpty());
    }

    void typeTextRejectsBadKeysAtOnce()
    {
        QLineEdit edit;
        edit.show();
        Probe probe;
        const QJsonObject response = request(probe, u"typeText"_s, {{u"handle"_s, refOf(probe, &edit)}, {u"text"_s, u"a<Frobnicate>"_s}});
        QCOMPARE(errorCode(response), u"BadRequest"_s);
        runPostedInput();
        QCOMPARE(edit.text(), QString());
    }

    void nativeTypeGoesToTheFocusWidget()
    {
        QWidget window;
        auto *layout = new QVBoxLayout(&window);
        auto *first = new QLineEdit(&window);
        auto *second = new QLineEdit(&window);
        layout->addWidget(first);
        layout->addWidget(second);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        second->setFocus();
        QTRY_COMPARE(QApplication::focusWidget(), second);
        Probe probe;
        QCOMPARE(result(request(probe, u"typeText"_s, {{u"text"_s, u"hi"_s}})), QJsonValue());
        QTRY_COMPARE(second->text(), u"hi"_s);
        QCOMPARE(first->text(), QString());
    }

    void nativeTypeEscapeClosesADialog()
    {
        QDialog dialog;
        auto *edit = new QLineEdit(&dialog);
        dialog.show();
        dialog.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&dialog));
        edit->setFocus();
        Probe probe;
        QCOMPARE(result(request(probe, u"typeText"_s, {{u"text"_s, u"<Escape>"_s}})), QJsonValue());
        QTRY_VERIFY(!dialog.isVisible());
        QCOMPARE(dialog.result(), int(QDialog::Rejected));
    }

    void clicksAButtonOnce()
    {
        QWidget window;
        auto *button = new QPushButton(u"Go"_s, &window);
        window.show();
        QSignalSpy clicked(button, &QPushButton::clicked);
        Probe probe;
        QCOMPARE(result(request(probe, u"mouseClick"_s, click(refOf(probe, button)))), QJsonValue());
        QTRY_COMPARE(clicked.count(), 1);
        QTest::qWait(50);
        QCOMPARE(clicked.count(), 1);
    }

    void rightClickWithModifiers()
    {
        QWidget window;
        window.show();
        struct Recorder : QObject
        {
            Qt::MouseButton button = Qt::NoButton;
            Qt::KeyboardModifiers mods;
            QPoint pos;
            bool eventFilter(QObject *, QEvent *event) override
            {
                if (event->type() == QEvent::MouseButtonPress) {
                    auto *mouse = static_cast<QMouseEvent *>(event);
                    button = mouse->button();
                    mods = mouse->modifiers();
                    pos = mouse->position().toPoint();
                }
                return false;
            }
        } recorder;
        auto *target = new QWidget(&window);
        target->setGeometry(10, 10, 100, 50);
        target->installEventFilter(&recorder);
        Probe probe;
        QJsonObject params = clickAt(refOf(probe, target), 0, 0);
        params.insert(u"button"_s, int(Qt::RightButton));
        params.insert(u"modifiers"_s, int(Qt::ShiftModifier));
        QCOMPARE(result(request(probe, u"mouseClick"_s, params)), QJsonValue());
        QTRY_COMPARE(recorder.button, Qt::RightButton);
        QCOMPARE(recorder.mods, Qt::ShiftModifier);
        // (0, 0) is the top left corner, not "the centre"
        QCOMPARE(recorder.pos, QPoint(0, 0));
    }

    void doubleClickOnATreeIndex()
    {
        TreeFixture fixture;
        QSignalSpy doubleClicked(fixture.tree, &QAbstractItemView::doubleClicked);
        Probe probe;
        const QJsonObject shares = fixture.index(probe, u"Shares"_s);
        QCOMPARE(result(request(probe, u"mouseClick"_s, click(shares, 2))), QJsonValue());
        QTRY_COMPARE(doubleClicked.count(), 1);
        QCOMPARE(doubleClicked.first().first().toModelIndex().data().toString(), u"Shares"_s);
    }

    void clickAt11TogglesTheCheckBox()
    {
        TreeFixture fixture;
        Probe probe;
        const QJsonObject personal = fixture.index(probe, u"Personal"_s);
        QCOMPARE(result(request(probe, u"mouseClick"_s, clickAt(personal, 11, 11))), QJsonValue());
        QTRY_COMPARE(fixture.personal->checkState(0), Qt::Unchecked);
        request(probe, u"mouseClick"_s, clickAt(personal, 11, 11));
        QTRY_COMPARE(fixture.personal->checkState(0), Qt::Checked);
    }

    void clickScrollsAnIndexIntoView()
    {
        QWidget window;
        auto *tree = new QTreeWidget(&window);
        tree->setGeometry(0, 0, 200, 100);
        for (int i = 0; i < 200; ++i) {
            new QTreeWidgetItem(tree, {u"row %1"_s.arg(i)});
        }
        window.resize(200, 100);
        window.show();
        QSignalSpy clicked(tree, &QAbstractItemView::clicked);
        Probe probe;
        const QJsonObject last = handleOf(find(probe, {{u"type"_s, u"QModelIndex"_s}, {u"container"_s, refOf(probe, tree)}, {u"text"_s, u"row 199"_s}}));
        request(probe, u"mouseClick"_s, click(last));
        QTRY_COMPARE(clicked.count(), 1);
        QCOMPARE(clicked.first().first().toModelIndex().data().toString(), u"row 199"_s);
    }

    void mouseClickRejectsBadParameters()
    {
        TreeFixture fixture;
        Probe probe;
        const QJsonObject shares = fixture.index(probe, u"Shares"_s);
        QCOMPARE(errorCode(request(probe, u"mouseClick"_s, click(shares, 3))), u"BadRequest"_s);
        QJsonObject params = click(shares);
        params.insert(u"button"_s, 3);
        QCOMPARE(errorCode(request(probe, u"mouseClick"_s, params)), u"BadRequest"_s);
        params = click(shares);
        params.insert(u"x"_s, 5);
        QCOMPARE(errorCode(request(probe, u"mouseClick"_s, params)), u"BadRequest"_s);
        // folder1 is collapsed below Personal, so it has no rectangle to click
        const QJsonObject folder1 =
            handleOf(find(probe, {{u"type"_s, u"QModelIndex"_s}, {u"container"_s, fixture.index(probe, u"Personal"_s)}, {u"text"_s, u"folder1"_s}}));
        QVERIFY(folder1.value(u"$handle"_s).isDouble());
        QCOMPARE(errorCode(request(probe, u"mouseClick"_s, click(folder1))), u"BadRequest"_s);
        QCOMPARE(errorCode(request(probe, u"mouseClick"_s, click(QJsonObject{{u"$handle"_s, 9999}}))), u"NoSuchObject"_s);
    }

    void clicksAnActionInAnOpenMenu()
    {
        QMenu menu;
        QAction *settings = menu.addAction(u"Settings"_s);
        menu.addAction(u"Quit"_s);
        QSignalSpy triggered(settings, &QAction::triggered);
        menu.popup(QPoint(20, 20));
        QVERIFY(QTest::qWaitForWindowExposed(&menu));
        Probe probe;
        QCOMPARE(result(request(probe, u"mouseClick"_s, click(refOf(probe, settings)))), QJsonValue());
        QTRY_COMPARE(triggered.count(), 1);
        QTRY_VERIFY(!menu.isVisible());
    }

    void mouseClickOnAHiddenActionIsBadRequest()
    {
        QAction action(u"Nowhere"_s);
        Probe probe;
        QCOMPARE(errorCode(request(probe, u"mouseClick"_s, click(refOf(probe, &action)))), u"BadRequest"_s);
    }

    void clicksAndTypesIntoQtQuickItems()
    {
        QQuickWidget quick;
        quick.setResizeMode(QQuickWidget::SizeRootObjectToView);
        quick.setSource(QUrl(u"qrc:/tst_actions.qml"_s));
        QCOMPARE(quick.status(), QQuickWidget::Ready);
        quick.resize(300, 200);
        quick.show();
        quick.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&quick));
        auto *tapTarget = quick.rootObject()->findChild<QQuickItem *>(u"tapTarget"_s);
        auto *input = quick.rootObject()->findChild<QQuickItem *>(u"quickInput"_s);
        QVERIFY(tapTarget && input);
        Probe probe;
        QCOMPARE(result(request(probe, u"mouseClick"_s, click(refOf(probe, tapTarget)))), QJsonValue());
        QTRY_COMPARE(tapTarget->property("clicks").toInt(), 1);
        QCOMPARE(result(request(probe, u"typeText"_s, {{u"handle"_s, refOf(probe, input)}, {u"text"_s, u"hey"_s}})), QJsonValue());
        QTRY_COMPARE(input->property("text").toString(), u"hey"_s);
    }

    void activatesAnAction()
    {
        QAction action(u"Force sync"_s);
        QSignalSpy triggered(&action, &QAction::triggered);
        Probe probe;
        QCOMPARE(result(request(probe, u"activateItem"_s, {{u"handle"_s, refOf(probe, &action)}})), QJsonValue());
        QTRY_COMPARE(triggered.count(), 1);
    }

    void activatesAMenuActionByClickingIt()
    {
        QMenu menu;
        menu.addAction(u"Edit"_s);
        QAction *remove = menu.addAction(u"Remove"_s);
        QSignalSpy triggered(remove, &QAction::triggered);
        menu.popup(QPoint(20, 20));
        QVERIFY(QTest::qWaitForWindowExposed(&menu));
        Probe probe;
        QCOMPARE(result(request(probe, u"activateItem"_s, {{u"handle"_s, refOf(probe, remove)}})), QJsonValue());
        QTRY_COMPARE(triggered.count(), 1);
        // a click closes the menu, QAction::trigger() would not
        QTRY_VERIFY(!menu.isVisible());
    }

    void activatesAnIndex()
    {
        TreeFixture fixture;
        QSignalSpy activated(fixture.tree, &QAbstractItemView::activated);
        Probe probe;
        QCOMPARE(result(request(probe, u"activateItem"_s, {{u"handle"_s, fixture.index(probe, u"Shares"_s)}})), QJsonValue());
        QTRY_COMPARE(activated.count(), 1);
        QCOMPARE(fixture.tree->currentIndex().data().toString(), u"Shares"_s);
    }

    void activateItemRejectsOtherTargets()
    {
        QWidget widget;
        Probe probe;
        QCOMPARE(errorCode(request(probe, u"activateItem"_s, {{u"handle"_s, refOf(probe, &widget)}})), u"BadRequest"_s);
    }

    void callsAllowlistedModelMethods()
    {
        TreeFixture fixture;
        Probe probe;
        const QJsonObject model = result(request(probe, u"call"_s, {{u"handle"_s, refOf(probe, fixture.tree)}, {u"name"_s, u"model"_s}})).toObject();
        QCOMPARE(model.value(u"$handle"_s).isDouble(), true);
        auto call = [&](const QJsonObject &target, const QString &name, const QJsonArray &args = {}) {
            return result(request(probe, u"call"_s, {{u"handle"_s, target}, {u"name"_s, name}, {u"args"_s, args}}));
        };
        QCOMPARE(call(handleOf(model), u"rowCount"_s), QJsonValue(2));
        QCOMPARE(call(handleOf(model), u"columnCount"_s), QJsonValue(1));
        const QJsonObject first = call(handleOf(model), u"index"_s, {0, 0}).toObject();
        QCOMPARE(first.value(u"$class"_s).toString(), u"QModelIndex"_s);
        QCOMPARE(call(handleOf(model), u"data"_s, {handleOf(first)}), QJsonValue(u"Personal"_s));
        QCOMPARE(call(handleOf(model), u"data"_s, {handleOf(first), int(Qt::CheckStateRole)}), QJsonValue(int(Qt::Checked)));
        QCOMPARE(call(handleOf(model), u"rowCount"_s, {handleOf(first)}), QJsonValue(1));
        const QJsonObject child = call(handleOf(model), u"index"_s, {0, 0, handleOf(first)}).toObject();
        QCOMPARE(call(handleOf(model), u"data"_s, {handleOf(child)}), QJsonValue(u"folder1"_s));

        // allowlisted names are methods for the proxy
        const QJsonObject asProperty = request(probe, u"get"_s, {{u"handle"_s, refOf(probe, fixture.tree)}, {u"name"_s, u"model"_s}});
        QCOMPARE(errorCode(asProperty), u"NoSuchProperty"_s);
        QCOMPARE(asProperty.value(u"error"_s).toObject().value(u"isMethod"_s), QJsonValue(true));
        const QJsonObject rowCount = request(probe, u"get"_s, {{u"handle"_s, handleOf(model)}, {u"name"_s, u"rowCount"_s}});
        QCOMPARE(rowCount.value(u"error"_s).toObject().value(u"isMethod"_s), QJsonValue(true));

        // a bad argument is a BadRequest
        QCOMPARE(errorCode(request(probe, u"call"_s, {{u"handle"_s, handleOf(model)}, {u"name"_s, u"index"_s}, {u"args"_s, QJsonArray{u"x"_s, 0}}})),
            u"BadRequest"_s);
    }

    void callsSlotsOnly()
    {
        QPointer<QWidget> widget = new QWidget;
        Probe probe;
        const QJsonObject ref = refOf(probe, widget);
        const QJsonObject setParent = request(probe, u"call"_s, {{u"handle"_s, ref}, {u"name"_s, u"setParent"_s}, {u"args"_s, QJsonArray{QJsonValue()}}});
        QCOMPARE(errorCode(setParent), u"MethodNotAllowed"_s);
        const QString message = setParent.value(u"error"_s).toObject().value(u"message"_s).toString();
        QVERIFY2(message.contains(u"QWidget"_s) && message.contains(u"setParent"_s), qPrintable(message));
        QCOMPARE(errorCode(request(probe, u"call"_s, {{u"handle"_s, ref}, {u"name"_s, u"model"_s}})), u"MethodNotAllowed"_s);
        // a slot with the wrong number of arguments
        QCOMPARE(errorCode(request(probe, u"call"_s, {{u"handle"_s, ref}, {u"name"_s, u"close"_s}, {u"args"_s, QJsonArray{1}}})), u"MethodNotAllowed"_s);
        // slots with arguments and return values
        widget->show();
        QCOMPARE(result(request(probe, u"call"_s, {{u"handle"_s, ref}, {u"name"_s, u"setWindowTitle"_s}, {u"args"_s, QJsonArray{u"Titled"_s}}})), QJsonValue());
        QCOMPARE(widget->windowTitle(), u"Titled"_s);
        QCOMPARE(result(request(probe, u"call"_s, {{u"handle"_s, ref}, {u"name"_s, u"close"_s}})), QJsonValue(true));
        QVERIFY(!widget->isVisible());

        const QJsonObject deleteLater = request(probe, u"call"_s, {{u"handle"_s, ref}, {u"name"_s, u"deleteLater"_s}});
        QVERIFY2(!deleteLater.contains(u"error"_s), qPrintable(QString::fromUtf8(QJsonDocument(deleteLater).toJson())));
        QCOMPARE(deleteLater.value(u"result"_s), QJsonValue());
        QTRY_VERIFY(widget.isNull());
    }

    void servesGettersAsProperties()
    {
        QWidget window;
        auto *button = new QToolButton(&window);
        auto *action = new QAction(u"account"_s, &window);
        action->setObjectName(u"accountAction_1"_s);
        button->setDefaultAction(action);
        auto *table = new QTableWidget(2, 2, &window);
        window.show();
        Probe probe;
        const QJsonObject defaultAction = result(request(probe, u"get"_s, {{u"handle"_s, refOf(probe, button)}, {u"name"_s, u"defaultAction"_s}})).toObject();
        QCOMPARE(defaultAction.value(u"$handle"_s), probe.registry().ref(action).value(u"$handle"_s));
        QCOMPARE(result(request(probe, u"get"_s, {{u"handle"_s, handleOf(defaultAction)}, {u"name"_s, u"objectName"_s}})), QJsonValue(u"accountAction_1"_s));
        auto *plain = new QToolButton(&window);
        QCOMPARE(result(request(probe, u"get"_s, {{u"handle"_s, refOf(probe, plain)}, {u"name"_s, u"defaultAction"_s}})), QJsonValue(Registry::nullRef()));

        const QJsonObject horizontal = find(probe, {{u"type"_s, u"QHeaderView"_s}, {u"orientation"_s, 1}, {u"window"_s, refOf(probe, &window)}});
        QCOMPARE(horizontal.value(u"$handle"_s), probe.registry().ref(table->horizontalHeader()).value(u"$handle"_s));
        const QJsonObject vertical = find(probe, {{u"type"_s, u"QHeaderView"_s}, {u"orientation"_s, 2}, {u"window"_s, refOf(probe, &window)}});
        QCOMPARE(vertical.value(u"$handle"_s), probe.registry().ref(table->verticalHeader()).value(u"$handle"_s));
        QCOMPARE(result(request(probe, u"get"_s, {{u"handle"_s, handleOf(vertical)}, {u"name"_s, u"orientation"_s}})), QJsonValue(2));
    }

    void activeModalWidget()
    {
        Probe probe;
        QCOMPARE(result(request(probe, u"activeModalWidget"_s)), QJsonValue(Registry::nullRef()));
        QMessageBox box(QMessageBox::Question, u"Confirm"_s, u"Really?"_s, QMessageBox::Ok);
        box.setWindowModality(Qt::ApplicationModal);
        box.show();
        const QJsonObject modal = result(request(probe, u"activeModalWidget"_s)).toObject();
        QCOMPARE(modal.value(u"$handle"_s), probe.registry().ref(&box).value(u"$handle"_s));
        QCOMPARE(modal.value(u"$class"_s).toString(), u"QMessageBox"_s);
    }

    void clipboardText()
    {
        QGuiApplication::clipboard()->setText(u"https://example.org/s/Grüße"_s);
        Probe probe;
        QCOMPARE(result(request(probe, u"clipboardText"_s)), QJsonValue(u"https://example.org/s/Grüße"_s));
    }

    void screenshot()
    {
        QWidget window;
        window.resize(100, 100);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(u"desktop.png"_s);
        Probe probe;
        QCOMPARE(result(request(probe, u"screenshot"_s, {{u"path"_s, path}})), QJsonValue());
        QVERIFY(!QImage(path).isNull());
        QCOMPARE(errorCode(request(probe, u"screenshot"_s, {{u"path"_s, dir.filePath(u"missing/dir/desktop.png"_s)}})), u"BadRequest"_s);
        QCOMPARE(errorCode(request(probe, u"screenshot"_s)), u"BadRequest"_s);
    }

    // The client waits for each answer: a request after input must see the input's effects.
    void requestsAfterInputSeeItsEffects()
    {
        QWidget window;
        auto *edit = new QLineEdit(&window);
        window.show();
        Probe probe;
        TransportClient client(probe);
        const QJsonObject ref = refOf(probe, edit);
        const int typeId = client.nextId();
        const int getId = client.nextId();
        // both lines arrive at once, as a pipelining client would send them
        probe.receive(TransportClient::line(typeId, u"typeText"_s, {{u"handle"_s, ref}, {u"text"_s, u"abc"_s}})
            + TransportClient::line(getId, u"get"_s, {{u"handle"_s, ref}, {u"name"_s, u"text"_s}}));
        QVERIFY(client.response(typeId));
        QCOMPARE(client.response(typeId)->value(u"result"_s), QJsonValue());
        // the get waits for the typed keys
        QVERIFY(!client.response(getId));
        QTRY_VERIFY(client.response(getId));
        QCOMPARE(client.response(getId)->value(u"result"_s), QJsonValue(u"abc"_s));

        // and the same for a click followed by a request in a later chunk
        auto *button = new QPushButton(u"Go"_s, &window);
        button->setCheckable(true);
        button->show();
        const QJsonObject buttonRef = refOf(probe, button);
        const int clickId = client.send(u"mouseClick"_s, click(buttonRef));
        QVERIFY(client.response(clickId));
        const int checkedId = client.send(u"get"_s, {{u"handle"_s, buttonRef}, {u"name"_s, u"checked"_s}});
        QVERIFY(!client.response(checkedId));
        QTRY_VERIFY(client.response(checkedId));
        QCOMPARE(client.response(checkedId)->value(u"result"_s), QJsonValue(true));
    }

    // A click that opens a modal dialog is answered at once, and the dialog's event loop serves further requests.
    void clickOpeningAModalDialogDoesNotBlock()
    {
        QWidget window;
        auto *button = new QPushButton(u"Quit"_s, &window);
        bool execReturned = false;
        int answer = -1;
        connect(button, &QPushButton::clicked, &window, [&] {
            QMessageBox box(QMessageBox::Question, u"Confirm"_s, u"Really quit?"_s, QMessageBox::Ok | QMessageBox::Cancel, &window);
            answer = box.exec();
            execReturned = true;
        });
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        // fail rather than hang if the click blocks or the probe never answers inside the box's event loop
        QTimer::singleShot(5000, &window, [] {
            if (auto *modal = qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
                qWarning("closing the modal dialog the probe did not close");
                modal->reject();
            }
        });
        Probe probe;
        TransportClient client(probe);
        QElapsedTimer elapsed;
        elapsed.start();
        const int clickId = client.send(u"mouseClick"_s, click(refOf(probe, button)));
        QVERIFY(client.response(clickId));
        QCOMPARE(client.response(clickId)->value(u"result"_s), QJsonValue());
        QVERIFY(elapsed.elapsed() < 1000);
        // the client asks at once, before the click has run: the request waits, then the box's event loop answers it
        const int early = client.send(u"activeModalWidget"_s);
        QVERIFY(!client.response(early));
        int pending = early;

        // a client script, driven by a timer that fires inside the message box's event loop
        enum Step { FindBox, FindOk, ClickOk, WaitClosed, Done };
        Step step = FindBox;
        QJsonObject box;
        QJsonObject ok;
        bool boxSeenWhileExecRuns = false;
        QTimer script;
        script.setInterval(10);
        bool earlyAnsweredWhileExecRuns = false;
        connect(&script, &QTimer::timeout, &window, [&] {
            if (pending == early && client.response(early)) {
                earlyAnsweredWhileExecRuns = !execReturned;
            }
            if (pending >= 0 && !client.response(pending)) {
                return;
            }
            const QJsonValue answered = pending >= 0 ? client.response(pending)->value(u"result"_s) : QJsonValue();
            switch (step) {
            case FindBox:
                if (pending >= 0 && answered.toObject().value(u"$class"_s) == u"QMessageBox"_s) {
                    box = handleOf(answered);
                    boxSeenWhileExecRuns = !execReturned;
                    step = FindOk;
                    pending = client.send(
                        u"find"_s, {{u"selector"_s, QJsonObject{{u"type"_s, u"QPushButton"_s}, {u"text"_s, u"OK"_s}, {u"window"_s, box}}}, {u"ready"_s, true}});
                    return;
                }
                pending = client.send(u"activeModalWidget"_s);
                return;
            case FindOk:
                if (!answered.toObject().value(u"$handle"_s).isDouble()) {
                    pending = client.send(
                        u"find"_s, {{u"selector"_s, QJsonObject{{u"type"_s, u"QPushButton"_s}, {u"text"_s, u"OK"_s}, {u"window"_s, box}}}, {u"ready"_s, true}});
                    return;
                }
                ok = handleOf(answered);
                step = ClickOk;
                pending = client.send(u"mouseClick"_s, click(ok));
                return;
            case ClickOk:
                step = WaitClosed;
                pending = client.send(u"activeModalWidget"_s);
                return;
            case WaitClosed:
                if (answered.toObject().value(u"$handle"_s).isNull()) {
                    step = Done;
                    script.stop();
                    return;
                }
                pending = client.send(u"activeModalWidget"_s);
                return;
            case Done:
                return;
            }
        });
        script.start();
        QTRY_COMPARE_WITH_TIMEOUT(step, Done, 8000);
        QVERIFY(earlyAnsweredWhileExecRuns);
        QVERIFY(boxSeenWhileExecRuns);
        QVERIFY(execReturned);
        QCOMPARE(answer, int(QMessageBox::Ok));
    }
};

QTEST_MAIN(TestActions)
#include "tst_actions.moc"
