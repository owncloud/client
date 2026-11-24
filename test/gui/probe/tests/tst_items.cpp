// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#include "items.h"
#include "probe.h"
#include "registry.h"
#include "selector.h"

#include <QDialog>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTabBar>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTest>
#include <QTreeView>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <memory>

using namespace QtGuiProbe;

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

QJsonValue get(Probe &probe, const QJsonObject &ref, const char *name)
{
    return result(request(probe, QStringLiteral("get"), {{QStringLiteral("handle"), ref}, {QStringLiteral("name"), QString::fromLatin1(name)}}));
}

QJsonArray childrenOf(Probe &probe, const QJsonObject &ref)
{
    return result(request(probe, QStringLiteral("children"), {{QStringLiteral("handle"), ref}})).toArray();
}

QModelIndex indexOf(const Target &target)
{
    const auto *index = std::get_if<QPersistentModelIndex>(&target);
    return index ? QModelIndex(*index) : QModelIndex();
}

const SyntheticItem *itemOf(const Target &target)
{
    return std::get_if<SyntheticItem>(&target);
}

// A window holding a selective-sync style tree with a "Name"/"Size" header.
struct TreeFixture
{
    QWidget window;
    QTreeWidget *tree = new QTreeWidget(&window);
    QTreeWidgetItem *personal = nullptr;
    QTreeWidgetItem *folder1 = nullptr;
    QTreeWidgetItem *folder2 = nullptr;
    QTreeWidgetItem *shares = nullptr;

    TreeFixture()
    {
        window.setObjectName(QStringLiteral("window"));
        tree->setObjectName(QStringLiteral("tree"));
        (new QVBoxLayout(&window))->addWidget(tree);
        tree->setHeaderLabels({QStringLiteral("Name"), QStringLiteral("Size")});
        personal = new QTreeWidgetItem(tree, {QStringLiteral("Personal"), QStringLiteral("26 B")});
        personal->setCheckState(0, Qt::Checked);
        folder1 = new QTreeWidgetItem(personal, {QStringLiteral("folder1 (13 B)"), QStringLiteral("13 B")});
        folder1->setCheckState(0, Qt::Unchecked);
        folder2 = new QTreeWidgetItem(personal, {QStringLiteral("folder2 (13 B)"), QStringLiteral("13 B")});
        folder2->setCheckState(0, Qt::PartiallyChecked);
        shares = new QTreeWidgetItem(tree, {QStringLiteral("Shares")});
        tree->expandAll();
        window.resize(400, 300);
    }

    void show()
    {
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
    }
};

const char treeSelector[] = R"_({"name":"tree","type":"QTreeWidget"})_";
const char personalSelector[] = R"_({"type":"QModelIndex","container":{"name":"tree","type":"QTreeWidget"},"column":"0","text":"Personal"})_";
const char folder1Selector[] = R"_({"type":"QModelIndex","column":"0","text":"folder1 (13 B)",
    "container":{"type":"QModelIndex","container":{"name":"tree","type":"QTreeWidget"},"column":"0","text":"Personal"}})_";
const char headerSelector[] = R"_({"type":"QHeaderView","container":{"name":"tree","type":"QTreeWidget"}})_";

// A tree view whose delegate shows one reused push button as the editor of a cell, as a "..." menu button.
struct EditorFixture
{
    struct ButtonDelegate : QStyledItemDelegate
    {
        QPushButton *button = new QPushButton(QStringLiteral("..."));
        ~ButtonDelegate() override
        {
            if (!button->parent()) {
                delete button;
            }
        }
        QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override
        {
            button->setParent(parent);
            button->setObjectName(QStringLiteral("buttonDelegateButton"));
            return button;
        }
        void destroyEditor(QWidget *, const QModelIndex &) const override { button->hide(); }
    };

    QWidget window;
    QTreeView *view = new QTreeView(&window);
    QStandardItemModel model{3, 2};
    ButtonDelegate delegate;

    EditorFixture()
    {
        view->setObjectName(QStringLiteral("folders"));
        (new QVBoxLayout(&window))->addWidget(view);
        for (int row = 0; row < model.rowCount(); ++row) {
            model.setItem(row, 0, new QStandardItem(QStringLiteral("folder %1").arg(row)));
            model.setItem(row, 1, new QStandardItem());
        }
        view->setModel(&model);
        view->setItemDelegateForColumn(1, &delegate);
        window.resize(500, 300);
    }

    ~EditorFixture() { view->setModel(nullptr); }

    void show()
    {
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
    }
};

} // namespace

class TestItems : public QObject
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
        registerItemSupport(*_engine, _registry);
    }

    void treeIndexesAndNestedContainers()
    {
        TreeFixture fixture;
        const auto personal = _engine->findAll(json(personalSelector));
        QCOMPARE(personal.size(), size_t(1));
        QCOMPARE(indexOf(personal.front()), fixture.tree->indexFromItem(fixture.personal, 0));

        const auto folder1 = _engine->findAll(json(folder1Selector));
        QCOMPARE(folder1.size(), size_t(1));
        QCOMPARE(indexOf(folder1.front()), fixture.tree->indexFromItem(fixture.folder1, 0));

        // only the direct rows of the container are searched
        QVERIFY(_engine->findAll(json(R"_({"type":"QModelIndex","container":{"name":"tree","type":"QTreeWidget"},"text":"folder1 (13 B)"})_")).empty());

        // without column: every column of the matching rows, in row-major order
        const auto row0 = _engine->findAll(json(R"_({"type":"QModelIndex","container":{"name":"tree","type":"QTreeWidget"},"row":0})_"));
        QCOMPARE(row0.size(), size_t(2));
        QCOMPARE(indexOf(row0[0]).column(), 0);
        QCOMPARE(indexOf(row0[1]).column(), 1);
        QCOMPARE(indexOf(row0[1]).data().toString(), QStringLiteral("26 B"));

        // patterns, occurrence
        const auto folders = _engine->findAll(json(R"_({"type":"QModelIndex","column":0,"text":{"$wildcard":"folder*"},
            "container":{"type":"QModelIndex","container":{"name":"tree","type":"QTreeWidget"},"column":"0","text":"Personal"}})_"));
        QCOMPARE(folders.size(), size_t(2));
        const auto second = _engine->findAll(json(R"_({"type":"QModelIndex","column":0,"text":{"$regex":"^folder"},"occurrence":2,
            "container":{"type":"QModelIndex","container":{"name":"tree","type":"QTreeWidget"},"column":"0","text":"Personal"}})_"));
        QCOMPARE(second.size(), size_t(1));
        QCOMPARE(indexOf(second.front()), fixture.tree->indexFromItem(fixture.folder2, 0));

        // the container must be a view or an index; a header is not a view of items
        QVERIFY(_engine->findAll(json(R"_({"type":"QModelIndex","container":{"name":"window"}})_")).empty());
        QVERIFY(_engine->findAll(json(R"_({"type":"QModelIndex"})_")).empty());
        QVERIFY(_engine->findAll(json(R"_({"type":"QModelIndex","container":{"type":"QHeaderView","container":{"name":"tree"}}})_")).empty());
    }

    void viewRootIndexIsTheScope()
    {
        TreeFixture fixture;
        fixture.tree->setRootIndex(fixture.tree->indexFromItem(fixture.personal, 0));
        const auto found = _engine->findAll(json(R"_({"type":"QModelIndex","container":{"name":"tree","type":"QTreeWidget"},"column":0})_"));
        QCOMPARE(found.size(), size_t(2));
        QCOMPARE(indexOf(found.front()), fixture.tree->indexFromItem(fixture.folder1, 0));
    }

    void tableCellByRowAndColumn()
    {
        QTableWidget table(3, 7);
        table.setObjectName(QStringLiteral("table"));
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 7; ++column) {
                table.setItem(row, column, new QTableWidgetItem(QStringLiteral("r%1c%2").arg(row).arg(column)));
            }
        }
        const auto found = _engine->findAll(json(R"_({"type":"QModelIndex","container":{"name":"table","type":"QTableWidget"},"row":1,"column":6})_"));
        QCOMPARE(found.size(), size_t(1));
        QCOMPARE(itemProperty(found.front(), QStringLiteral("text")), QJsonValue(QStringLiteral("r1c6")));
        QCOMPARE(itemProperty(found.front(), QStringLiteral("row")), QJsonValue(1));
        QCOMPARE(itemProperty(found.front(), QStringLiteral("column")), QJsonValue(6));
        // string values compare by value, out-of-range rows match nothing
        QCOMPARE(_engine->findAll(json(R"_({"type":"QModelIndex","container":{"name":"table"},"row":"1","column":"6"})_")).size(), size_t(1));
        QVERIFY(_engine->findAll(json(R"_({"type":"QModelIndex","container":{"name":"table"},"row":3,"column":6})_")).empty());
        QVERIFY(_engine->findAll(json(R"_({"type":"QModelIndex","container":{"name":"table"},"row":-1})_")).empty());
        // a column found by text gives the row back
        const auto byText = _engine->findAll(json(R"_({"type":"QModelIndex","container":{"name":"table"},"column":1,"text":"r2c1"})_"));
        QCOMPARE(byText.size(), size_t(1));
        QCOMPARE(itemProperty(byText.front(), QStringLiteral("row")), QJsonValue(2));
    }

    void tabItems()
    {
        QTabWidget tabs;
        tabs.setObjectName(QStringLiteral("tabs"));
        tabs.addTab(new QWidget, QStringLiteral("Local Activity"));
        tabs.addTab(new QWidget, QStringLiteral("Not Synced"));

        const auto byIndex = _engine->findAll(json(R"_({"type":"TabItem","container":{"name":"tabs","type":"QTabWidget"},"index":1})_"));
        QCOMPARE(byIndex.size(), size_t(1));
        QVERIFY(itemOf(byIndex.front()));
        QCOMPARE(itemOf(byIndex.front())->owner.data(), tabs.tabBar());
        QCOMPARE(itemProperty(byIndex.front(), QStringLiteral("text")), QJsonValue(QStringLiteral("Not Synced")));
        QCOMPARE(itemProperty(byIndex.front(), QStringLiteral("index")), QJsonValue(1));

        // the tab bar itself as container (Squish names it qt_tabwidget_tabbar), and by text
        const auto byText =
            _engine->findAll(json(R"_({"type":"TabItem","container":{"name":"qt_tabwidget_tabbar","type":"QTabBar"},"text":"Local Activity"})_"));
        QCOMPARE(byText.size(), size_t(1));
        QCOMPARE(itemOf(byText.front())->index, 0);
        QCOMPARE(_engine->findAll(json(R"_({"type":"TabItem","container":{"name":"tabs"}})_")).size(), size_t(2));
        QVERIFY(_engine->findAll(json(R"_({"type":"TabItem","container":{"name":"tabs"},"index":2})_")).empty());
    }

    void headerItems()
    {
        TreeFixture fixture;
        const auto size = _engine->findAll(json(R"_({"type":"HeaderViewItem","text":"Size","container":{"type":"QHeaderView","container":{"name":"tree"}}})_"));
        QCOMPARE(size.size(), size_t(1));
        QCOMPARE(itemOf(size.front())->owner.data(), fixture.tree->header());
        QCOMPARE(itemProperty(size.front(), QStringLiteral("section")), QJsonValue(1));
        QCOMPARE(itemProperty(size.front(), QStringLiteral("text")), QJsonValue(QStringLiteral("Size")));
        QCOMPARE(_engine->findAll(json(R"_({"type":"HeaderViewItem","section":0,"container":{"type":"QHeaderView","container":{"name":"tree"}}})_")).size(),
            size_t(1));

        // moved sections come in display order and keep their logical section
        fixture.tree->header()->moveSection(1, 0);
        QJsonObject allSections = json(R"_({"type":"HeaderViewItem"})_");
        allSections.insert(QStringLiteral("container"), json(headerSelector));
        const auto all = _engine->findAll(allSections);
        QCOMPARE(all.size(), size_t(2));
        QCOMPARE(itemProperty(all[0], QStringLiteral("section")), QJsonValue(1));
        QCOMPARE(itemProperty(all[1], QStringLiteral("section")), QJsonValue(0));

        // hidden sections are not shown
        fixture.tree->header()->setSectionHidden(1, true);
        QCOMPARE(_engine->findAll(json(R"_({"type":"HeaderViewItem","container":{"type":"QHeaderView","container":{"name":"tree"}}})_")).size(), size_t(1));
    }

    void visibleKeyOnItems()
    {
        TreeFixture fixture;
        const char selector[] = R"_({"type":"HeaderViewItem","text":"Size","visible":true,"container":{"type":"QHeaderView","container":{"name":"tree"}}})_";
        QVERIFY(_engine->findAll(json(selector)).empty());
        fixture.show();
        QCOMPARE(_engine->findAll(json(selector)).size(), size_t(1));
        const auto personal = _engine->findAll(json(personalSelector));
        QCOMPARE(itemProperty(personal.front(), QStringLiteral("visible")), QJsonValue(true));
        fixture.personal->setExpanded(false);
        const auto folder1 = _engine->findAll(json(folder1Selector));
        QCOMPARE(folder1.size(), size_t(1));
        QCOMPARE(itemProperty(folder1.front(), QStringLiteral("visible")), QJsonValue(false));
    }

    void indexProperties()
    {
        TreeFixture fixture;
        fixture.show();
        const Target personal = _engine->findAll(json(personalSelector)).front();
        const auto folders = _engine->findAll(json(R"_({"type":"QModelIndex","column":0,
            "container":{"type":"QModelIndex","container":{"name":"tree"},"column":"0","text":"Personal"}})_"));
        QCOMPARE(folders.size(), size_t(2));
        QCOMPARE(itemProperty(personal, QStringLiteral("checkState")), QJsonValue(QStringLiteral("checked")));
        QCOMPARE(itemProperty(folders[0], QStringLiteral("checkState")), QJsonValue(QStringLiteral("unchecked")));
        QCOMPARE(itemProperty(folders[1], QStringLiteral("checkState")), QJsonValue(QStringLiteral("partiallyChecked")));
        const auto sizeCell = _engine->findAll(json(R"_({"type":"QModelIndex","container":{"name":"tree"},"row":0,"column":1})_"));
        QCOMPARE(itemProperty(sizeCell.front(), QStringLiteral("checkState")), QJsonValue(QJsonValue::Null));
        QCOMPARE(itemProperty(personal, QStringLiteral("displayText")), QJsonValue(QStringLiteral("Personal")));

        QCOMPARE(itemProperty(folders[1], QStringLiteral("selected")), QJsonValue(false));
        fixture.tree->setCurrentItem(fixture.folder2);
        QCOMPARE(itemProperty(folders[1], QStringLiteral("selected")), QJsonValue(true));
        QCOMPARE(itemProperty(personal, QStringLiteral("selected")), QJsonValue(false));
        // selector keys read the same properties
        QCOMPARE(_engine
                     ->findAll(json(R"_({"type":"QModelIndex","column":0,"selected":true,
            "container":{"type":"QModelIndex","container":{"name":"tree"},"column":"0","text":"Personal"}})_"))
                     .size(),
            size_t(1));
        QCOMPARE(_engine->findAll(json(R"_({"type":"QModelIndex","checkState":"checked","container":{"name":"tree"}})_")).size(), size_t(1));

        QVERIFY(!itemProperty(personal, QStringLiteral("noSuchThing")));
        QVERIFY(!itemProperty(Target(static_cast<QObject *>(fixture.tree)), QStringLiteral("text")));
    }

    void readiness()
    {
        TreeFixture fixture;
        QTabWidget tabs;
        tabs.setObjectName(QStringLiteral("tabs"));
        tabs.addTab(new QWidget, QStringLiteral("One"));
        const char tabSelector[] = R"_({"type":"TabItem","container":{"name":"tabs"},"index":0})_";
        const char sectionSelector[] = R"_({"type":"HeaderViewItem","text":"Size","container":{"type":"QHeaderView","container":{"name":"tree"}}})_";

        // found while hidden, but not ready
        QVERIFY(_engine->find(json(personalSelector), false));
        QVERIFY(!_engine->find(json(personalSelector), true));
        QVERIFY(!_engine->find(json(tabSelector), true));
        QVERIFY(!_engine->find(json(sectionSelector), true));

        fixture.show();
        tabs.show();
        QVERIFY(QTest::qWaitForWindowExposed(&tabs));
        QVERIFY(_engine->find(json(personalSelector), true));
        QVERIFY(_engine->find(json(folder1Selector), true));
        QVERIFY(_engine->find(json(tabSelector), true));
        QVERIFY(_engine->find(json(sectionSelector), true));

        fixture.tree->setEnabled(false);
        tabs.tabBar()->setEnabled(false);
        QVERIFY(!_engine->find(json(personalSelector), true));
        QVERIFY(!_engine->find(json(tabSelector), true));
        QVERIFY(!_engine->find(json(sectionSelector), true));
    }

    void itemRects()
    {
        TreeFixture fixture;
        fixture.show();
        const Target personal = _engine->findAll(json(personalSelector)).front();
        QCOMPARE(viewOf(indexOf(personal)), fixture.tree);
        QCOMPARE(itemWidget(personal), fixture.tree->viewport());
        QCOMPARE(itemRect(personal), fixture.tree->visualRect(indexOf(personal)));
        QVERIFY(itemRect(personal).isValid());

        QHeaderView *header = fixture.tree->header();
        const Target size =
            _engine->findAll(json(R"_({"type":"HeaderViewItem","text":"Size","container":{"type":"QHeaderView","container":{"name":"tree"}}})_")).front();
        QCOMPARE(itemWidget(size), header->viewport());
        QCOMPARE(itemRect(size), QRect(header->sectionViewportPosition(1), 0, header->sectionSize(1), header->viewport()->height()));

        QTabWidget tabs;
        tabs.setObjectName(QStringLiteral("tabs"));
        tabs.addTab(new QWidget, QStringLiteral("Local Activity"));
        tabs.addTab(new QWidget, QStringLiteral("Not Synced"));
        tabs.show();
        QVERIFY(QTest::qWaitForWindowExposed(&tabs));
        const Target tab = _engine->findAll(json(R"_({"type":"TabItem","container":{"name":"tabs"},"index":1})_")).front();
        QCOMPARE(itemWidget(tab), tabs.tabBar());
        QCOMPARE(itemRect(tab), tabs.tabBar()->tabRect(1));

        // plain widgets: their own rect
        QCOMPARE(itemWidget(Target(static_cast<QObject *>(fixture.tree))), fixture.tree);
        QCOMPARE(itemRect(Target(static_cast<QObject *>(fixture.tree))), fixture.tree->rect());

        // collapsed: no rectangle
        fixture.personal->setExpanded(false);
        const Target folder1 = _engine->findAll(json(folder1Selector)).front();
        QVERIFY(!itemRect(folder1).isValid());
    }

    void viewOfPrefersTheViewItWasFoundThrough()
    {
        QStandardItemModel model(3, 1);
        QTableView first;
        first.setObjectName(QStringLiteral("first"));
        first.setModel(&model);
        QTableView second;
        second.setObjectName(QStringLiteral("second"));
        second.setModel(&model);
        const auto viaSecond = _engine->findAll(json(R"_({"type":"QModelIndex","container":{"name":"second"},"row":1})_"));
        QCOMPARE(viewOf(indexOf(viaSecond.front())), &second);
        const auto viaFirst = _engine->findAll(json(R"_({"type":"QModelIndex","container":{"name":"first"},"row":1})_"));
        QCOMPARE(viewOf(indexOf(viaFirst.front())), &first);
        // an index of a model no view shows has no view
        QStandardItemModel orphan(1, 1);
        QCOMPARE(viewOf(orphan.index(0, 0)), nullptr);
    }

    void largeModelsFindAllRows()
    {
        const int rows = 2500;
        QStandardItemModel model(rows, 3);
        for (int row = 0; row < rows; ++row) {
            model.setItem(row, 0, new QStandardItem(QString::number(row)));
        }
        QTableView view;
        view.setObjectName(QStringLiteral("big"));
        view.setModel(&model);
        const auto found = _engine->findAll(json(R"_({"type":"QModelIndex","container":{"name":"big"},"column":0})_"));
        QCOMPARE(found.size(), size_t(rows));
        QCOMPARE(indexOf(found.back()).row(), rows - 1);

        // two views of one model are two scopes; their common indexes count once
        QTableView other;
        other.setObjectName(QStringLiteral("big"));
        other.setModel(&model);
        QCOMPARE(_engine->findAll(json(R"_({"type":"QModelIndex","container":{"name":"big"},"column":0})_")).size(), size_t(rows));
        // and two tab bars stay two sets of tabs
        QTabBar barA;
        barA.setObjectName(QStringLiteral("bar"));
        barA.addTab(QStringLiteral("a"));
        QTabBar barB;
        barB.setObjectName(QStringLiteral("bar"));
        barB.addTab(QStringLiteral("b"));
        QCOMPARE(_engine->findAll(json(R"_({"type":"TabItem","container":{"name":"bar"}})_")).size(), size_t(2));
    }

    void registryDeduplicatesItems()
    {
        TreeFixture fixture;
        const Target personal = _engine->findAll(json(personalSelector)).front();
        const QJsonObject a = _registry.ref(personal);
        const QJsonObject b = _registry.ref(fixture.tree->indexFromItem(fixture.personal, 0));
        QCOMPARE(a.value(QStringLiteral("$class")), QJsonValue(QStringLiteral("QModelIndex")));
        QCOMPARE(a, b);
        QVERIFY(a != _registry.ref(fixture.tree->indexFromItem(fixture.personal, 1)));

        QTabBar bar;
        bar.addTab(QStringLiteral("a"));
        bar.addTab(QStringLiteral("b"));
        const QJsonObject tab = _registry.ref(SyntheticItem{&bar, QStringLiteral("TabItem"), 1});
        QCOMPARE(tab, _registry.ref(SyntheticItem{&bar, QStringLiteral("TabItem"), 1}));
        QVERIFY(tab != _registry.ref(SyntheticItem{&bar, QStringLiteral("TabItem"), 0}));
        QVERIFY(tab != _registry.ref(SyntheticItem{&bar, QStringLiteral("HeaderViewItem"), 1}));

        // an index keeps its handle when the rows before it go away
        const QJsonObject folder2 = _registry.ref(fixture.tree->indexFromItem(fixture.folder2, 0));
        delete fixture.folder1;
        fixture.folder1 = nullptr;
        const QJsonObject moved = _registry.ref(fixture.tree->indexFromItem(fixture.folder2, 0));
        QCOMPARE(moved, folder2);

        _registry.clear();
        QVERIFY(_registry.ref(personal) != a);
    }

    // over the protocol: find, get, children and handle identity
    // Squish's rowIndex and columnIndex: the cell a widget inside an item view (an editor, an index widget) sits on.
    void widgetsInsideAViewHaveRowAndColumnIndex()
    {
        EditorFixture fixture;
        fixture.show();
        fixture.view->openPersistentEditor(fixture.model.index(1, 1));
        QTRY_VERIFY(fixture.delegate.button->isVisible());
        Probe probe;
        QJsonObject selector = json(R"_({"columnIndex":1,"rowIndex":1,"container":{"name":"folders","type":"QTreeView"},
            "name":"buttonDelegateButton","type":"QPushButton","visible":1})_");
        const QJsonObject button = find(probe, selector);
        QCOMPARE(button.value(QStringLiteral("$handle")).isDouble(), true);
        QCOMPARE(get(probe, button, "rowIndex"), QJsonValue(1));
        QCOMPARE(get(probe, button, "columnIndex"), QJsonValue(1));
        selector.insert(QStringLiteral("rowIndex"), 0);
        QVERIFY(find(probe, selector).value(QStringLiteral("$handle")).isNull());
        selector.insert(QStringLiteral("rowIndex"), 1);
        selector.insert(QStringLiteral("columnIndex"), 0);
        QVERIFY(find(probe, selector).value(QStringLiteral("$handle")).isNull());
    }

    void nestedIndexWidgetChildHasRowAndColumnIndex()
    {
        EditorFixture fixture;
        auto *cell = new QWidget;
        auto *inner = new QPushButton(QStringLiteral("inner"), cell);
        inner->setObjectName(QStringLiteral("inner"));
        (new QVBoxLayout(cell))->addWidget(inner);
        fixture.view->setIndexWidget(fixture.model.index(0, 0), cell);
        fixture.show();
        QTRY_VERIFY(inner->isVisible());
        Probe probe;
        const QJsonObject ref = find(probe, json(R"_({"name":"inner","rowIndex":0,"columnIndex":0})_"));
        QCOMPARE(ref.value(QStringLiteral("$handle")).isDouble(), true);
    }

    // A window parented to an editor (the menu or dialog a delegate button opens) is not inside the view.
    void windowsParentedToAnEditorHaveNoRowIndex()
    {
        EditorFixture fixture;
        fixture.show();
        fixture.view->openPersistentEditor(fixture.model.index(1, 1));
        QTRY_VERIFY(fixture.delegate.button->isVisible());
        auto *dialog = new QDialog(fixture.delegate.button);
        dialog->setObjectName(QStringLiteral("dialog"));
        auto *inside = new QPushButton(QStringLiteral("ok"), dialog);
        inside->setObjectName(QStringLiteral("inside"));
        // at the screen's top left, so a walk across the window boundary maps it onto the editor's cell
        dialog->setGeometry(0, 0, 20, 20);
        inside->setGeometry(0, 0, 20, 20);
        dialog->show();
        QVERIFY(QTest::qWaitForWindowExposed(dialog));
        Probe probe;
        for (const char *name : {"dialog", "inside"}) {
            const QJsonObject ref = find(probe, {{QStringLiteral("name"), QString::fromLatin1(name)}});
            QVERIFY(ref.value(QStringLiteral("$handle")).isDouble());
            for (const QString key : {QStringLiteral("rowIndex"), QStringLiteral("columnIndex")}) {
                const QJsonObject response = request(probe, QStringLiteral("get"), {{QStringLiteral("handle"), ref}, {QStringLiteral("name"), key}});
                QVERIFY2(response.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString() == QStringLiteral("NoSuchProperty"),
                    QJsonDocument(response).toJson().constData());
            }
        }
        delete dialog;
    }

    void widgetsOutsideViewsHaveNoRowIndex()
    {
        EditorFixture fixture;
        auto *outside = new QPushButton(QStringLiteral("outside"), &fixture.window);
        outside->setObjectName(QStringLiteral("outside"));
        fixture.window.layout()->addWidget(outside);
        fixture.show();
        Probe probe;
        const QJsonObject ref = find(probe, json(R"_({"name":"outside"})_"));
        const QJsonObject response =
            request(probe, QStringLiteral("get"), {{QStringLiteral("handle"), ref}, {QStringLiteral("name"), QStringLiteral("rowIndex")}});
        QCOMPARE(response.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString(), QStringLiteral("NoSuchProperty"));
        QVERIFY(find(probe, json(R"_({"name":"outside","rowIndex":0})_")).value(QStringLiteral("$handle")).isNull());
        // the view and its viewport are not inside the viewport
        QVERIFY(find(probe, json(R"_({"name":"folders","rowIndex":0})_")).value(QStringLiteral("$handle")).isNull());
    }

    void probeServesItems()
    {
        TreeFixture fixture;
        fixture.show();
        Probe probe;
        const QJsonObject personal = find(probe, json(personalSelector));
        QCOMPARE(personal.value(QStringLiteral("$class")), QJsonValue(QStringLiteral("QModelIndex")));
        QCOMPARE(find(probe, json(personalSelector)), personal);
        QCOMPARE(find(probe, json(personalSelector), true), personal);
        QCOMPARE(get(probe, personal, "text"), QJsonValue(QStringLiteral("Personal")));
        QCOMPARE(get(probe, personal, "row"), QJsonValue(0));
        QCOMPARE(get(probe, personal, "checkState"), QJsonValue(QStringLiteral("checked")));
        const QJsonObject noSuch =
            request(probe, QStringLiteral("get"), {{QStringLiteral("handle"), personal}, {QStringLiteral("name"), QStringLiteral("nope")}});
        QCOMPARE(noSuch.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")), QJsonValue(QStringLiteral("NoSuchProperty")));

        const QJsonObject folder1 = find(probe, json(folder1Selector));
        QCOMPARE(get(probe, folder1, "text"), QJsonValue(QStringLiteral("folder1 (13 B)")));

        // children of a view: its top-level indexes first, then its QObject children
        const QJsonObject tree = find(probe, json(treeSelector));
        const QJsonArray treeChildren = childrenOf(probe, tree);
        QVERIFY(treeChildren.size() > 2);
        QCOMPARE(treeChildren[0].toObject(), personal);
        QCOMPARE(get(probe, treeChildren[0].toObject(), "text"), QJsonValue(QStringLiteral("Personal")));
        QCOMPARE(get(probe, treeChildren[1].toObject(), "text"), QJsonValue(QStringLiteral("Shares")));
        QVERIFY(treeChildren[2].toObject().value(QStringLiteral("$class")) != QJsonValue(QStringLiteral("QModelIndex")));
        QCOMPARE(childrenOf(probe, tree), treeChildren);

        // children of an index: its column-0 children
        const QJsonArray personalChildren = childrenOf(probe, personal);
        QCOMPARE(personalChildren.size(), 2);
        QCOMPARE(personalChildren[0].toObject(), folder1);
        QCOMPARE(get(probe, personalChildren[1].toObject(), "text"), QJsonValue(QStringLiteral("folder2 (13 B)")));
        QCOMPARE(childrenOf(probe, personal), personalChildren);
        QCOMPARE(childrenOf(probe, folder1).size(), 0);

        // header sections and tabs over the protocol
        const QJsonObject size = find(probe, json(R"_({"type":"HeaderViewItem","text":"Size","visible":true,
            "container":{"type":"QHeaderView","container":{"name":"tree"},"visible":1}})_"),
            true);
        QCOMPARE(size.value(QStringLiteral("$class")), QJsonValue(QStringLiteral("HeaderViewItem")));
        QCOMPARE(get(probe, size, "section"), QJsonValue(1));
        QCOMPARE(find(probe, json(R"_({"type":"HeaderViewItem","section":1,"container":{"type":"QHeaderView","container":{"name":"tree"}}})_")), size);

        QTabWidget tabs;
        tabs.setObjectName(QStringLiteral("tabs"));
        tabs.addTab(new QWidget, QStringLiteral("Local Activity"));
        tabs.addTab(new QWidget, QStringLiteral("Not Synced"));
        const QJsonObject tab = find(probe, json(R"_({"type":"TabItem","container":{"name":"tabs"},"index":1})_"));
        QCOMPARE(tab.value(QStringLiteral("$class")), QJsonValue(QStringLiteral("TabItem")));
        QCOMPARE(get(probe, tab, "text"), QJsonValue(QStringLiteral("Not Synced")));
        QCOMPARE(get(probe, tab, "index"), QJsonValue(1));
        QCOMPARE(find(probe, json(R"_({"type":"TabItem","container":{"name":"tabs"},"text":"Not Synced"})_")), tab);

        // a handle to an index works as a container
        QJsonObject byHandle = json(R"_({"type":"QModelIndex","column":0,"text":"folder1 (13 B)"})_");
        byHandle.insert(QStringLiteral("container"), personal);
        QCOMPARE(find(probe, byHandle), folder1);
    }
};

QTEST_MAIN(TestItems)
#include "tst_items.moc"
