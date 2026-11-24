// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

#include "items.h"

#include "quick.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QApplication>
#include <QHash>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWidget>
#include <QTabBar>
#include <QTabWidget>

#include <cmath>

namespace QtGuiProbe {

namespace {

    const QString modelIndexType = QStringLiteral("QModelIndex");
    const QString tabItemType = QStringLiteral("TabItem");
    const QString headerViewItemType = QStringLiteral("HeaderViewItem");

    // The view each model was last searched or listed through, so an index knows the view it came from.
    QHash<const QAbstractItemModel *, QPointer<QAbstractItemView>> &lastViews()
    {
        static QHash<const QAbstractItemModel *, QPointer<QAbstractItemView>> views;
        return views;
    }

    // An item view that shows model items; a header is a view of the model too, but of its header data.
    QAbstractItemView *itemViewOf(QObject *object)
    {
        auto *view = qobject_cast<QAbstractItemView *>(object);
        return view && !qobject_cast<QHeaderView *>(view) && view->model() ? view : nullptr;
    }

    void rememberView(QAbstractItemView *view)
    {
        lastViews().insert(view->model(), view);
    }

    QObject *objectOf(const Target &target)
    {
        const auto *object = std::get_if<QObject *>(&target);
        return object ? *object : nullptr;
    }

    // The integer a plain selector value stands for ("0" or 0), std::nullopt for patterns and anything else.
    std::optional<int> plainInteger(const QJsonValue &value)
    {
        if (value.isDouble()) {
            const double number = value.toDouble();
            if (std::trunc(number) == number && std::abs(number) < 1e9) {
                return static_cast<int>(number);
            }
            return std::nullopt;
        }
        if (value.isString()) {
            bool ok = false;
            const int number = value.toString().trimmed().toInt(&ok);
            if (ok) {
                return number;
            }
        }
        return std::nullopt;
    }

    // The rows or columns worth trying: just the one a plain value names, else all of them.
    std::pair<int, int> candidateRange(const QJsonObject &sel, const QString &key, int count)
    {
        const QJsonValue value = sel.value(key);
        if (value.isUndefined()) {
            return {0, count};
        }
        if (const auto wanted = plainInteger(value)) {
            if (*wanted < 0 || *wanted >= count) {
                return {0, 0};
            }
            return {*wanted, *wanted + 1};
        }
        return {0, count};
    }

    std::vector<Target> searchIndexes(SelectorEngine &engine, const QJsonObject &sel, const QAbstractItemModel *model, const QModelIndex &parent)
    {
        std::vector<Target> found;
        const auto [firstRow, endRow] = candidateRange(sel, QStringLiteral("row"), model->rowCount(parent));
        const auto [firstColumn, endColumn] = candidateRange(sel, QStringLiteral("column"), model->columnCount(parent));
        for (int row = firstRow; row < endRow; ++row) {
            for (int column = firstColumn; column < endColumn; ++column) {
                const QModelIndex index = model->index(row, column, parent);
                if (!index.isValid()) {
                    continue;
                }
                Target target = QPersistentModelIndex(index);
                if (engine.matchesProperties(target, sel)) {
                    found.push_back(std::move(target));
                }
            }
        }
        return found;
    }

    std::vector<Target> resolveIndexes(SelectorEngine &engine, const QJsonObject &sel, const Target &scope)
    {
        if (const auto *index = std::get_if<QPersistentModelIndex>(&scope)) {
            if (!index->isValid()) {
                return {};
            }
            return searchIndexes(engine, sel, index->model(), *index);
        }
        if (auto *view = itemViewOf(objectOf(scope))) {
            rememberView(view);
            return searchIndexes(engine, sel, view->model(), view->rootIndex());
        }
        return {};
    }

    QTabBar *tabBarOf(QObject *object)
    {
        if (auto *tabWidget = qobject_cast<QTabWidget *>(object)) {
            return tabWidget->tabBar();
        }
        return qobject_cast<QTabBar *>(object);
    }

    std::vector<Target> resolveTabs(SelectorEngine &engine, const QJsonObject &sel, const Target &scope)
    {
        QTabBar *bar = tabBarOf(objectOf(scope));
        if (!bar) {
            return {};
        }
        std::vector<Target> found;
        for (int i = 0; i < bar->count(); ++i) {
            Target target = SyntheticItem{bar, tabItemType, i};
            if (engine.matchesProperties(target, sel)) {
                found.push_back(std::move(target));
            }
        }
        return found;
    }

    std::vector<Target> resolveSections(SelectorEngine &engine, const QJsonObject &sel, const Target &scope)
    {
        auto *header = qobject_cast<QHeaderView *>(objectOf(scope));
        if (!header || !header->model()) {
            return {};
        }
        std::vector<Target> found;
        // display order; a section's identity is its logical index
        for (int visual = 0; visual < header->count(); ++visual) {
            const int section = header->logicalIndex(visual);
            if (section < 0 || header->isSectionHidden(section)) {
                continue;
            }
            Target target = SyntheticItem{header, headerViewItemType, section};
            if (engine.matchesProperties(target, sel)) {
                found.push_back(std::move(target));
            }
        }
        return found;
    }

    QJsonValue checkStateName(const QVariant &value)
    {
        if (!value.isValid()) {
            return QJsonValue::Null;
        }
        switch (value.value<Qt::CheckState>()) {
        case Qt::Checked:
            return QStringLiteral("checked");
        case Qt::PartiallyChecked:
            return QStringLiteral("partiallyChecked");
        case Qt::Unchecked:
            break;
        }
        return QStringLiteral("unchecked");
    }

    std::optional<QJsonValue> indexProperty(const QModelIndex &index, const QString &name)
    {
        if (name == QLatin1String("text") || name == QLatin1String("displayText")) {
            return QJsonValue(index.data(Qt::DisplayRole).toString());
        }
        if (name == QLatin1String("row")) {
            return QJsonValue(index.row());
        }
        if (name == QLatin1String("column")) {
            return QJsonValue(index.column());
        }
        if (name == QLatin1String("checkState")) {
            return checkStateName(index.data(Qt::CheckStateRole));
        }
        if (name == QLatin1String("selected")) {
            const QAbstractItemView *view = viewOf(index);
            return QJsonValue(view && view->selectionModel() && view->selectionModel()->isSelected(index));
        }
        if (name == QLatin1String("visible")) {
            const QAbstractItemView *view = viewOf(index);
            return QJsonValue(view && view->isVisible() && view->visualRect(index).intersects(view->viewport()->rect()));
        }
        return std::nullopt;
    }

    std::optional<QJsonValue> tabProperty(QTabBar *bar, int index, const QString &name)
    {
        if (index < 0 || index >= bar->count()) {
            return std::nullopt;
        }
        if (name == QLatin1String("text")) {
            return QJsonValue(bar->tabText(index));
        }
        if (name == QLatin1String("index")) {
            return QJsonValue(index);
        }
        if (name == QLatin1String("visible")) {
            return QJsonValue(bar->isVisible() && bar->isTabVisible(index) && bar->tabRect(index).intersects(bar->rect()));
        }
        return std::nullopt;
    }

    QRect sectionRect(const QHeaderView *header, int section)
    {
        if (section < 0 || section >= header->count() || header->isSectionHidden(section)) {
            return {};
        }
        const QWidget *viewport = header->viewport();
        if (header->orientation() == Qt::Horizontal) {
            return QRect(header->sectionViewportPosition(section), 0, header->sectionSize(section), viewport->height());
        }
        return QRect(0, header->sectionViewportPosition(section), viewport->width(), header->sectionSize(section));
    }

    std::optional<QJsonValue> sectionProperty(QHeaderView *header, int section, const QString &name)
    {
        if (!header->model() || section < 0 || section >= header->count()) {
            return std::nullopt;
        }
        if (name == QLatin1String("text")) {
            return QJsonValue(header->model()->headerData(section, header->orientation(), Qt::DisplayRole).toString());
        }
        if (name == QLatin1String("section")) {
            return QJsonValue(section);
        }
        if (name == QLatin1String("visible")) {
            return QJsonValue(header->isVisible() && sectionRect(header, section).intersects(header->viewport()->rect()));
        }
        return std::nullopt;
    }

    // The cell a widget inside an item view (an editor, an index widget or a child of one) sits on: the index under
    // the widget's centre in the innermost view's viewport. Invalid outside views and where no cell is.
    // indexAt() rather than the view's editor bookkeeping: the index -> editor map (indexWidget()) has no public
    // reverse, and delegates lay editors out over their cell (updateEditorGeometry), so the centre is in that cell.
    // The walk stays inside the widget's window: a menu or dialog parented to an editor is not inside the view, and
    // mapTo() across a window boundary would add the window's screen position. A viewport is never a window.
    QModelIndex cellOf(const QWidget *widget)
    {
        if (widget->isWindow()) {
            return {};
        }
        for (const QWidget *parent = widget->parentWidget(); parent && !parent->isWindow(); parent = parent->parentWidget()) {
            QAbstractItemView *view = itemViewOf(parent->parentWidget());
            if (view && view->viewport() == parent) {
                return view->indexAt(widget->mapTo(parent, widget->rect().center()));
            }
        }
        return {};
    }

    // Squish's rowIndex and columnIndex of a widget inside an item view, std::nullopt for other names and targets.
    std::optional<QJsonValue> cellProperty(const Target &target, const QString &name)
    {
        const bool row = name == QLatin1String("rowIndex");
        if (!row && name != QLatin1String("columnIndex")) {
            return std::nullopt;
        }
        const auto *widget = qobject_cast<const QWidget *>(objectOf(target));
        const QModelIndex cell = widget ? cellOf(widget) : QModelIndex();
        if (!cell.isValid()) {
            return std::nullopt;
        }
        return QJsonValue(row ? cell.row() : cell.column());
    }

    bool widgetReady(const QWidget *widget)
    {
        return widget && widget->isVisible() && widget->isEnabled();
    }

} // namespace

void registerItemSupport(SelectorEngine &engine, Registry &registry)
{
    Q_UNUSED(registry) // handles of items need nothing beyond the Registry's built-in support
    engine.addTypeResolver([&engine](const QJsonObject &sel, const Target &scope) -> std::optional<std::vector<Target>> {
        const QString type = sel.value(QStringLiteral("type")).toString();
        if (type == modelIndexType) {
            return resolveIndexes(engine, sel, scope);
        }
        if (type == tabItemType) {
            return resolveTabs(engine, sel, scope);
        }
        if (type == headerViewItemType) {
            return resolveSections(engine, sel, scope);
        }
        return std::nullopt;
    });
    engine.addPropertyReader([](const Target &target, const QString &name) { return itemProperty(target, name); });
    engine.addPropertyReader([](const Target &target, const QString &name) { return cellProperty(target, name); });
    engine.addReadyCheck([](const Target &target) -> std::optional<bool> {
        if (const auto *index = std::get_if<QPersistentModelIndex>(&target)) {
            return index->isValid() && widgetReady(viewOf(*index));
        }
        if (const auto *item = std::get_if<SyntheticItem>(&target)) {
            if (item->kind == tabItemType || item->kind == headerViewItemType) {
                return widgetReady(qobject_cast<QWidget *>(item->owner.data()));
            }
        }
        return std::nullopt;
    });
}

std::optional<QJsonValue> itemProperty(const Target &target, const QString &name)
{
    if (const auto *index = std::get_if<QPersistentModelIndex>(&target)) {
        if (!index->isValid()) {
            return std::nullopt;
        }
        return indexProperty(*index, name);
    }
    if (const auto *item = std::get_if<SyntheticItem>(&target)) {
        if (item->kind == tabItemType) {
            if (auto *bar = qobject_cast<QTabBar *>(item->owner.data())) {
                return tabProperty(bar, item->index, name);
            }
        } else if (item->kind == headerViewItemType) {
            if (auto *header = qobject_cast<QHeaderView *>(item->owner.data())) {
                return sectionProperty(header, item->index, name);
            }
        }
    }
    return std::nullopt;
}

QList<Target> itemChildren(const Target &target)
{
    const QAbstractItemModel *model = nullptr;
    QModelIndex parent;
    if (const auto *index = std::get_if<QPersistentModelIndex>(&target)) {
        if (!index->isValid()) {
            return {};
        }
        model = index->model();
        parent = *index;
    } else if (auto *view = itemViewOf(objectOf(target))) {
        rememberView(view);
        model = view->model();
        parent = view->rootIndex();
    } else {
        return {};
    }
    QList<Target> children;
    const int rows = model->rowCount(parent);
    children.reserve(rows);
    for (int row = 0; row < rows; ++row) {
        const QModelIndex child = model->index(row, 0, parent);
        if (child.isValid()) {
            children.append(QPersistentModelIndex(child));
        }
    }
    return children;
}

QAbstractItemView *viewOf(const QModelIndex &index)
{
    const QAbstractItemModel *model = index.model();
    if (!model) {
        return nullptr;
    }
    const auto known = lastViews().constFind(model);
    if (known != lastViews().cend()) {
        if (known.value() && known.value()->model() == model) {
            return known.value();
        }
        lastViews().erase(known);
    }
    // some view of the model, a visible one if there is one
    QAbstractItemView *fallback = nullptr;
    const auto widgets = QApplication::allWidgets();
    for (auto *widget : widgets) {
        auto *view = itemViewOf(widget);
        if (!view || view->model() != model) {
            continue;
        }
        if (view->isVisible()) {
            return view;
        }
        if (!fallback) {
            fallback = view;
        }
    }
    return fallback;
}

QWidget *itemWidget(const Target &target)
{
    if (const auto *index = std::get_if<QPersistentModelIndex>(&target)) {
        QAbstractItemView *view = index->isValid() ? viewOf(*index) : nullptr;
        return view ? view->viewport() : nullptr;
    }
    if (const auto *item = std::get_if<SyntheticItem>(&target)) {
        if (item->kind == tabItemType) {
            return qobject_cast<QTabBar *>(item->owner.data());
        }
        if (item->kind == headerViewItemType) {
            auto *header = qobject_cast<QHeaderView *>(item->owner.data());
            return header ? header->viewport() : nullptr;
        }
        return nullptr;
    }
    QObject *object = std::get<QObject *>(target);
    if (const auto *item = qobject_cast<const QQuickItem *>(object)) {
        return quickHost(item);
    }
    return qobject_cast<QWidget *>(object);
}

QRect itemRect(const Target &target)
{
    if (const auto *index = std::get_if<QPersistentModelIndex>(&target)) {
        QAbstractItemView *view = index->isValid() ? viewOf(*index) : nullptr;
        return view ? view->visualRect(*index) : QRect();
    }
    if (const auto *item = std::get_if<SyntheticItem>(&target)) {
        if (item->kind == tabItemType) {
            auto *bar = qobject_cast<QTabBar *>(item->owner.data());
            return bar ? bar->tabRect(item->index) : QRect();
        }
        if (item->kind == headerViewItemType) {
            auto *header = qobject_cast<QHeaderView *>(item->owner.data());
            return header ? sectionRect(header, item->index) : QRect();
        }
        return {};
    }
    const QObject *object = std::get<QObject *>(target);
    if (const auto *item = qobject_cast<const QQuickItem *>(object)) {
        return quickItemRect(item);
    }
    const auto *widget = qobject_cast<const QWidget *>(object);
    return widget ? widget->rect() : QRect();
}

} // namespace QtGuiProbe
