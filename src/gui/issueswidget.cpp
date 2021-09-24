/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include <QtGui>
#include <QtWidgets>

#include "issueswidget.h"
#include "configfile.h"
#include "syncresult.h"
#include "syncengine.h"
#include "logger.h"
#include "theme.h"
#include "folderman.h"
#include "syncfileitem.h"
#include "folder.h"
#include "models/models.h"
#include "openfilemanager.h"
#include "protocolwidget.h"
#include "accountstate.h"
#include "account.h"
#include "accountmanager.h"
#include "common/syncjournalfilerecord.h"
#include "elidedlabel.h"


#include "ui_issueswidget.h"

#include <climits>

namespace {
bool persistsUntilLocalDiscovery(const OCC::ProtocolItem &data)
{
    return data.status() == OCC::SyncFileItem::Conflict
        || (data.status() == OCC::SyncFileItem::FileIgnored && data.direction() == OCC::SyncFileItem::Up)
        || data.status() == OCC::SyncFileItem::Excluded;
}

}
namespace OCC {

class SyncFileItemStatusSetSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    using StatusSet = std::array<bool, SyncFileItem::StatusCount>;

    explicit SyncFileItemStatusSetSortFilterProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
        resetFilter();
    }

    ~SyncFileItemStatusSetSortFilterProxyModel() override
    {
    }

    StatusSet filter() const
    {
        return _filter;
    }

    void setFilter(const StatusSet &newFilter)
    {
        if (_filter != newFilter) {
            _filter = newFilter;
            invalidateFilter();
        }
    }

    void resetFilter()
    {
        StatusSet defaultFilter;
        defaultFilter.fill(true);
        defaultFilter[SyncFileItem::NoStatus] = false;
        defaultFilter[SyncFileItem::Success] = false;
        setFilter(defaultFilter);
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        QModelIndex idx = sourceModel()->index(sourceRow, filterKeyColumn(), sourceParent);

        bool ok = false;
        int sourceData = sourceModel()->data(idx, filterRole()).toInt(&ok);
        if (!ok) {
            return false;
        }

        return _filter[static_cast<SyncFileItem::Status>(sourceData)];
    }

private:
    StatusSet _filter;
};

/**
 * If more issues are reported than this they will not show up
 * to avoid performance issues around sorting this many issues.
 */

IssuesWidget::IssuesWidget(QWidget *parent)
    : QWidget(parent)
    , _ui(new Ui::IssuesWidget)
{
    _ui->setupUi(this);

    connect(ProgressDispatcher::instance(), &ProgressDispatcher::progressInfo,
        this, &IssuesWidget::slotProgressInfo);
    connect(ProgressDispatcher::instance(), &ProgressDispatcher::itemCompleted,
        this, &IssuesWidget::slotItemCompleted);
    connect(ProgressDispatcher::instance(), &ProgressDispatcher::syncError,
        this, [this](const QString &folderAlias, const QString &message, ErrorCategory) {
            auto item = SyncFileItemPtr::create();
            item->_status = SyncFileItem::NormalError;
            item->_errorString = message;
            _model->addProtocolItem(ProtocolItem { folderAlias, item });
        });

    connect(ProgressDispatcher::instance(), &ProgressDispatcher::excluded, this, [this](Folder *f, const QString &file, CSYNC_EXCLUDE_TYPE reason) {
        auto item = SyncFileItemPtr::create();
        item->_status = SyncFileItem::FileIgnored;
        item->_file = file;
        switch (reason) {
        case CSYNC_FILE_EXCLUDE_RESERVED:
            item->_errorString = tr("The file %1 was ignored as its name is reserved by %2").arg(file, Theme::instance()->appNameGUI());
            break;
        default:
            Q_UNREACHABLE();
        }
        _model->addProtocolItem(ProtocolItem { f, item });
    });

    _model = new ProtocolItemModel(20000, true, this);
    _sortModel = new QSortFilterProxyModel(this);
    _sortModel->setSourceModel(_model);
    _statusSortModel = new SyncFileItemStatusSetSortFilterProxyModel(this);
    _statusSortModel->setSourceModel(_sortModel);
    _statusSortModel->setSortRole(Qt::DisplayRole); // Sorting should be done based on the text in the column cells, but...
    _statusSortModel->setFilterRole(Models::UnderlyingDataRole); // ... filtering should be done on the underlying enum value.
    _statusSortModel->setFilterKeyColumn(static_cast<int>(ProtocolItemModel::ProtocolItemRole::Status));
    _ui->_tableView->setModel(_statusSortModel);

    auto header = new ExpandingHeaderView(QStringLiteral("ActivityErrorListHeaderV2"), _ui->_tableView);
    _ui->_tableView->setHorizontalHeader(header);
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setExpandingColumn(static_cast<int>(ProtocolItemModel::ProtocolItemRole::Action));
    header->setSortIndicator(static_cast<int>(ProtocolItemModel::ProtocolItemRole::Time), Qt::DescendingOrder);

    connect(_ui->_tableView, &QTreeView::customContextMenuRequested, this, &IssuesWidget::slotItemContextMenu);
    _ui->_tableView->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header, &QHeaderView::customContextMenuRequested, [this, header]() {
        auto menu = showFilterMenu(header);
        menu->addAction(tr("Reset column sizes"), header, [header] { header->resizeColumns(true); });
    });

    connect(_ui->_filterButton, &QAbstractButton::clicked, this, [this] {
        showFilterMenu(_ui->_filterButton);
    });

    _ui->_tooManyIssuesWarning->hide();
    connect(_model, &ProtocolItemModel::rowsInserted, this, [this] {
        Q_EMIT issueCountUpdated(_model->rowCount());
        _ui->_tooManyIssuesWarning->setVisible(_model->isModelFull());
    });
    connect(_model, &ProtocolItemModel::modelReset, this, [this] {
        Q_EMIT issueCountUpdated(_model->rowCount());
        _ui->_tooManyIssuesWarning->setVisible(_model->isModelFull());
    });

    _ui->_conflictHelp->hide();
    _ui->_conflictHelp->setText(
        tr("There were conflicts. <a href=\"%1\">Check the documentation on how to resolve them.</a>")
            .arg(Theme::instance()->conflictHelpUrl()));

    connect(FolderMan::instance(), &FolderMan::folderRemoved, this, [this](Folder *f) {
        _model->remove_if([f](const ProtocolItem &item) {
            return item.folder() == f;
        });
    });
}

IssuesWidget::~IssuesWidget()
{
    delete _ui;
}

QMenu *IssuesWidget::showFilterMenu(QWidget *parent)
{
    auto menu = new QMenu(parent);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    auto accountFilterReset = Models::addFilterMenuItems(menu, AccountManager::instance()->accountNames(), _sortModel, static_cast<int>(ProtocolItemModel::ProtocolItemRole::Account), tr("Account"), Qt::DisplayRole);
    menu->addSeparator();
    auto statusFilterReset = addStatusFilter(menu);
    menu->addSeparator();
    addResetFiltersAction(menu, { accountFilterReset, statusFilterReset });

    QTimer::singleShot(0, menu, [menu] {
        menu->popup(QCursor::pos());
    });

    return menu;
}

void IssuesWidget::addResetFiltersAction(QMenu *menu, const QList<std::function<void()>> &resetFunctions)
{
    menu->addAction(QCoreApplication::translate("OCC::Models", "Reset Filters"), [resetFunctions]() {
        for (const auto &reset : resetFunctions) {
            reset();
        }
    });
}

void IssuesWidget::slotProgressInfo(const QString &folder, const ProgressInfo &progress)
{
    if (progress.status() == ProgressInfo::Reconcile) {
        // Wipe all non-persistent entries - as well as the persistent ones
        // in cases where a local discovery was done.
        auto f = FolderMan::instance()->folder(folder);
        if (!f)
            return;
        const auto &engine = f->syncEngine();
        const auto style = engine.lastLocalDiscoveryStyle();
        _model->remove_if([&](const ProtocolItem &item) {
            if (item.folder() != f) {
                return false;
            }
            if (item.direction() == SyncFileItem::None && item.status() != SyncFileItem::Excluded) {
                // TODO: don't clear syncErrors and excludes for now.
                // make them either unique or remove them on the next sync?
                return false;
            }
            if (style == LocalDiscoveryStyle::FilesystemOnly) {
                return true;
            }
            if (!persistsUntilLocalDiscovery(item)) {
                return true;
            }
            // Definitely wipe the entry if the file no longer exists
            if (!QFileInfo::exists(f->path() + item.path())) {
                return true;
            }

            auto path = QFileInfo(item.path()).dir().path();
            if (path == QLatin1Char('.'))
                path.clear();

            return engine.shouldDiscoverLocally(path);
        });
    }
    if (progress.status() == ProgressInfo::Done) {
        // We keep track very well of pending conflicts.
        // Inform other components about them.
        QStringList conflicts;
        for (const auto &data : _model->rawData()) {
            if (data.folder()->path() == folder
                && data.status() == SyncFileItem::Conflict) {
                conflicts.append(data.path());
            }
        }
        emit ProgressDispatcher::instance()->folderConflicts(folder, conflicts);

        _ui->_conflictHelp->setHidden(Theme::instance()->conflictHelpUrl().isEmpty() || conflicts.isEmpty());
    }
}

void IssuesWidget::slotItemCompleted(const QString &folder, const SyncFileItemPtr &item)
{
    if (!item->showInIssuesTab())
        return;
    _model->addProtocolItem(ProtocolItem { folder, item });
}

void IssuesWidget::slotItemContextMenu()
{
    auto rows = _ui->_tableView->selectionModel()->selectedRows();
    for (int i = 0; i < rows.size(); ++i) {
        rows[i] = _statusSortModel->mapToSource(rows[i]);
        rows[i] = _sortModel->mapToSource(rows[i]);
    }
    ProtocolWidget::showContextMenu(this, _model, rows);
}

std::function<void(void)> IssuesWidget::addStatusFilter(QMenu *menu)
{
    menu->addAction(QCoreApplication::translate("OCC::Models", "Status Filter:"))->setEnabled(false);

    // Use a QActionGroup to contain all status filter items, so we can find them back easily to reset.
    auto statusFilterGroup = new QActionGroup(menu);
    statusFilterGroup->setExclusive(false);

    const auto initialFilter = _statusSortModel->filter();

    { // Add all errors under 1 action:
        const std::array<SyncFileItem::Status, 5> ErrorStatusItems = {
            SyncFileItem::Status::FatalError,
            SyncFileItem::Status::NormalError,
            SyncFileItem::Status::SoftError,
            SyncFileItem::Status::DetailError
        };

        auto action = menu->addAction(SyncFileItem::statusEnumDisplayName(SyncFileItem::NormalError), [this, ErrorStatusItems](bool checked) {
            auto currentFilter = _statusSortModel->filter();
            for (const auto &item : ErrorStatusItems) {
                currentFilter[item] = checked;
            }
            _statusSortModel->setFilter(currentFilter);
        });
        action->setCheckable(true);
        action->setChecked(initialFilter[ErrorStatusItems[0]]);
        statusFilterGroup->addAction(action);
    }

    // Add the other non-error items:
    const std::array<SyncFileItem::Status, 5> OtherStatusItems = {
        SyncFileItem::Status::Conflict,
        SyncFileItem::Status::FileIgnored,
        SyncFileItem::Status::Restoration,
        SyncFileItem::Status::BlacklistedError,
        SyncFileItem::Status::Excluded
    };
    for (const auto &item : OtherStatusItems) {
        auto action = menu->addAction(SyncFileItem::statusEnumDisplayName(item), [this, item](bool checked) {
            auto currentFilter = _statusSortModel->filter();
            currentFilter[item] = checked;
            _statusSortModel->setFilter(currentFilter);
        });
        action->setCheckable(true);
        action->setChecked(initialFilter[item]);
        statusFilterGroup->addAction(action);
    }

    menu->addSeparator();

    // Add action to reset all filters at once:
    return [statusFilterGroup, this]() {
        for (QAction *action : statusFilterGroup->actions()) {
            action->setChecked(true);
        }
        _statusSortModel->resetFilter();
    };
}
}
