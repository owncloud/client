/*
 * Copyright (C) by Olivier Goffart <ogoffart@woboq.com>
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
#include "selectivesyncwidget.h"

#include "gui/folderman.h"
#include "libsync/configfile.h"
#include "libsync/networkjobs.h"
#include "libsync/theme.h"

#include "resources/resources.h"

#include <QHeaderView>
#include <QLabel>
#include <QScopedValueRollback>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace OCC {


class SelectiveSyncTreeViewItem : public QTreeWidgetItem
{
public:
    SelectiveSyncTreeViewItem(int type = QTreeWidgetItem::Type)
        : QTreeWidgetItem(type)
    {
    }
    SelectiveSyncTreeViewItem(const QStringList &strings, int type = QTreeWidgetItem::Type)
        : QTreeWidgetItem(strings, type)
    {
    }
    SelectiveSyncTreeViewItem(QTreeWidget *view, int type = QTreeWidgetItem::Type)
        : QTreeWidgetItem(view, type)
    {
    }
    SelectiveSyncTreeViewItem(QTreeWidgetItem *parent, int type = QTreeWidgetItem::Type)
        : QTreeWidgetItem(parent, type)
    {
    }

private:
    bool operator<(const QTreeWidgetItem &other) const override
    {
        int column = treeWidget()->sortColumn();
        if (column == 1) {
            return data(1, Qt::UserRole).toLongLong() < other.data(1, Qt::UserRole).toLongLong();
        }
        return QTreeWidgetItem::operator<(other);
    }
};

SelectiveSyncWidget::SelectiveSyncWidget(AccountPtr account, QWidget *parent)
    : QWidget(parent)
    , _account(account)
    , _inserting(false)
    , _folderTree(new QTreeWidget(this))
{
    _loading = new QLabel(tr("Loading ..."), _folderTree);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto header = new QLabel(this);
    header->setText(tr("Deselect remote folders you do not wish to synchronize."));
    header->setWordWrap(true);
    layout->addWidget(header);

    layout->addWidget(_folderTree);

    connect(_folderTree, &QTreeWidget::itemExpanded, this, &SelectiveSyncWidget::slotItemExpanded);
    connect(_folderTree, &QTreeWidget::itemChanged, this, &SelectiveSyncWidget::slotItemChanged);
    _folderTree->setSortingEnabled(true);
    _folderTree->sortByColumn(0, Qt::AscendingOrder);
    _folderTree->setColumnCount(2);
    _folderTree->header()->setSectionResizeMode(0, QHeaderView::QHeaderView::ResizeToContents);
    _folderTree->header()->setSectionResizeMode(1, QHeaderView::QHeaderView::ResizeToContents);
    _folderTree->header()->setStretchLastSection(true);
    _folderTree->headerItem()->setText(0, tr("Name"));
    _folderTree->headerItem()->setText(1, tr("Size"));

    ConfigFile::setupDefaultExcludeFilePaths(_excludedFiles);
    _excludedFiles.reloadExcludeFiles();
}

QSize SelectiveSyncWidget::sizeHint() const
{
    return QWidget::sizeHint().expandedTo(QSize(600, 600));
}

void SelectiveSyncWidget::refreshFolders()
{
    auto *job = new PropfindJob(_account, davUrl(), _folderPath, PropfindJob::Depth::One, this);
    job->setProperties({QByteArrayLiteral("resourcetype"), QByteArrayLiteral("http://owncloud.org/ns:size")});
    connect(job, &PropfindJob::directoryListingSubfolders, this, &SelectiveSyncWidget::slotUpdateDirectories);
    connect(job, &PropfindJob::finishedWithError, this, [job, this] {
        if (job->reply()->error() == QNetworkReply::ContentNotFoundError) {
            _loading->setText(tr("Currently there are no subfolders on the server."));
        } else {
            _loading->setText(tr("An error occurred while loading the list of subfolders."));
        }
        _loading->resize(_loading->sizeHint()); // because it's not in a layout
    });
    job->start();
    _folderTree->clear();
    _loading->show();
    _loading->move(10, _folderTree->header()->height() + 10);
}

void SelectiveSyncWidget::setFolderInfo(const QString &folderPath, const QString &rootName, const QSet<QString> &oldBlackList)
{
    _folderPath = Utility::stripTrailingSlash(folderPath);
    _rootName = rootName;
    _oldBlackList = oldBlackList;
    refreshFolders();
}

static QTreeWidgetItem *findFirstChild(QTreeWidgetItem *parent, const QString &text)
{
    for (int i = 0; i < parent->childCount(); ++i) {
        QTreeWidgetItem *child = parent->child(i);
        if (child->text(0) == text) {
            return child;
        }
    }
    return nullptr;
}

/**
 * @brief Recursively insert an item in the tree
 * @param parent The parent item into which a child is inserted
 * @param pathTrail The tail of the path that still needs to be inserted
 * @param path The full path
 * @param size The size of the folder
 * @param showChildIndicator False if it is known that a folder does not have any child items
 *        (and should not show the expansion triangle), true otherwise.
 *
 * The algorithm: for inserting "folder/subfolder",start with the root item, and pass in the path
 * both as list of (sub)folders, and the whole path as string. Then search for the already existing
 * "folder" item, and if not found, add it. Then recurse with this item as the parent, and "subfolder"
 * as the `pathTrail`. Repeat this again, and the last recursive call is with the item for "subfolder",
 * and an empty `pathTrail`. This item is used to set the full path as the tooltip, and set the
 * `showChildIndicator`.
 *
 * By default, the `showChildIndicator` is set to true. When the folder is expanded, and a PROPFIND
 * is done, *and* there are no child folders, a recursive insert is done for the path (which already
 * is in the tree), and the `showChildIndicator` set to false.
 *
 * @note Note about the `showChildIndicator`: if set to false, this will only be applied to the very
 *       last item of the path/pathTrail: all parent items need to be expandable in order to reach
 *       that last folder. So all parents of that folder will have expansion triangles shown.
 */
void SelectiveSyncWidget::recursiveInsert(QTreeWidgetItem *parent, QStringList pathTrail, QString path, qint64 size, bool showChildIndicator)
{
    if (pathTrail.size() == 0) {
        if (path.endsWith(QLatin1Char('/'))) {
            path.chop(1);
        }
        parent->setToolTip(0, path);
        parent->setData(0, Qt::UserRole, path);
        parent->setChildIndicatorPolicy(showChildIndicator ? QTreeWidgetItem::ShowIndicator : QTreeWidgetItem::DontShowIndicator);
    } else {
        SelectiveSyncTreeViewItem *item = static_cast<SelectiveSyncTreeViewItem *>(findFirstChild(parent, pathTrail.first()));
        if (!item) {
            item = new SelectiveSyncTreeViewItem(parent);
            if (parent->checkState(0) == Qt::Checked || parent->checkState(0) == Qt::PartiallyChecked) {
                item->setCheckState(0, Qt::Checked);
                for (const auto &str : std::as_const(_oldBlackList)) {
                    if (str == path || str == QLatin1Char('/')) {
                        item->setCheckState(0, Qt::Unchecked);
                        break;
                    } else if (str.startsWith(path)) {
                        item->setCheckState(0, Qt::PartiallyChecked);
                    }
                }
            } else if (parent->checkState(0) == Qt::Unchecked) {
                item->setCheckState(0, Qt::Unchecked);
            }
            item->setIcon(0, Resources::getCoreIcon(QStringLiteral("folder-sync")));
            item->setText(0, pathTrail.first());
            if (size >= 0) {
                item->setText(1, Utility::octetsToString(size));
                item->setData(1, Qt::UserRole, size);
            }

            // Mark all sub-folders as potentially having child folders. When expanded, a PROPFIND
            // will be done, and sub-folders will be added. If no sub-folders are found by the PROPFIND,
            // the triangle will be hidden.
            item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        }

        pathTrail.removeFirst();
        recursiveInsert(item, pathTrail, path, size, showChildIndicator);
    }
}

void SelectiveSyncWidget::slotUpdateDirectories(QStringList list)
{
    auto job = qobject_cast<PropfindJob *>(sender());
    QScopedValueRollback<bool> isInserting(_inserting, true);

    SelectiveSyncTreeViewItem *root = static_cast<SelectiveSyncTreeViewItem *>(_folderTree->topLevelItem(0));

    const QString rootPath = Utility::ensureTrailingSlash(Utility::concatUrlPath(davUrl(), _folderPath).path());
    const bool ignoreHiddenFiles = FolderMan::instance()->ignoreHiddenFiles();

    // Check for excludes.
    list.erase(std::remove_if(list.begin(), list.end(),
                   [&rootPath, ignoreHiddenFiles, this](
                       const QString &it) { return _excludedFiles.isExcludedRemote(it, rootPath, ignoreHiddenFiles, ItemTypeDirectory); }),
        list.end());

    QStringList relativeList;
    relativeList.reserve(list.size());
    for (const QString &path : std::as_const(list)) {
        Q_ASSERT(path.startsWith(rootPath));
        const QString relativePath = path.mid(rootPath.size());
        if (!relativePath.isEmpty()) {
            relativeList.append(relativePath);
        }
    }

    // Since / cannot be in the blacklist, expand it to the actual
    // list of top-level folders as soon as possible.
    if (_oldBlackList.size() == 1 && _oldBlackList.contains(QLatin1String("/"))) {
        _oldBlackList.clear();
        for (const QString &path : std::as_const(relativeList)) {
            _oldBlackList.insert(path);
        }
    }

    if (!root && list.size() <= 1) {
        _loading->setText(tr("Currently there are no subfolders on the server."));
        _loading->resize(_loading->sizeHint()); // because it's not in a layout
        return;
    } else {
        _loading->hide();
    }

    if (!root) {
        root = new SelectiveSyncTreeViewItem(_folderTree);
        root->setText(0, _rootName);
        root->setIcon(0, Theme::instance()->applicationIcon());
        root->setData(0, Qt::UserRole, QString());
        root->setCheckState(0, Qt::Checked);
        qint64 size = job ? job->sizes().value(rootPath, -1) : -1;
        if (size >= 0) {
            root->setText(1, Utility::octetsToString(size));
            root->setData(1, Qt::UserRole, size);
        }
    }

    // The base directory of the propfind is always returned, so if there is just 1 entry, there
    // are no child folders. EXCEPT for the root directory, because that entry is filtered out of
    // the `relativeList` above. The case of no sub-folders in the root is handled above, so if
    // the original list contains the `rootPath`, we show the indicator.
    const bool showChildIndicator = relativeList.size() > 1 || list.contains(rootPath);

    Utility::sortFilenames(relativeList);
    for (const QString &path : std::as_const(relativeList)) {
        const auto size = job ? job->sizes().value(Utility::ensureTrailingSlash(Utility::concatUrlPathItems({rootPath, path}))) : 0;
        const QStringList paths = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
        if (paths.isEmpty()) {
            continue;
        }
        recursiveInsert(root, paths, Utility::ensureTrailingSlash(path), size, showChildIndicator);
    }

    // Root is partially checked if any children are not checked
    for (int i = 0; i < root->childCount(); ++i) {
        const auto child = root->child(i);
        if (child->checkState(0) != Qt::Checked) {
            root->setCheckState(0, Qt::PartiallyChecked);
            break;
        }
    }

    root->setExpanded(true);
}

void SelectiveSyncWidget::slotItemExpanded(QTreeWidgetItem *item)
{
    QString dir = item->data(0, Qt::UserRole).toString();
    if (dir.isEmpty()) {
        return;
    }
    PropfindJob *job = new PropfindJob(_account, davUrl(), Utility::concatUrlPathItems({_folderPath, dir}), PropfindJob::Depth::One, this);
    job->setProperties({QByteArrayLiteral("resourcetype"), QByteArrayLiteral("http://owncloud.org/ns:size")});
    connect(job, &PropfindJob::directoryListingSubfolders, this, &SelectiveSyncWidget::slotUpdateDirectories);
    job->start();
}

void SelectiveSyncWidget::slotItemChanged(QTreeWidgetItem *item, int col)
{
    if (col != 0 || _inserting)
        return;

    if (item->checkState(0) == Qt::Checked) {
        // If we are checked, check that we may need to check the parent as well if
        // all the siblings are also checked
        QTreeWidgetItem *parent = item->parent();
        if (parent && parent->checkState(0) != Qt::Checked) {
            bool hasUnchecked = false;
            for (int i = 0; i < parent->childCount(); ++i) {
                if (parent->child(i)->checkState(0) != Qt::Checked) {
                    hasUnchecked = true;
                    break;
                }
            }
            if (!hasUnchecked) {
                parent->setCheckState(0, Qt::Checked);
            } else if (parent->checkState(0) == Qt::Unchecked) {
                parent->setCheckState(0, Qt::PartiallyChecked);
            }
        }
        // also check all the children
        for (int i = 0; i < item->childCount(); ++i) {
            if (item->child(i)->checkState(0) != Qt::Checked) {
                item->child(i)->setCheckState(0, Qt::Checked);
            }
        }
    }

    if (item->checkState(0) == Qt::Unchecked) {
        QTreeWidgetItem *parent = item->parent();
        if (parent && parent->checkState(0) == Qt::Checked) {
            parent->setCheckState(0, Qt::PartiallyChecked);
        }

        // Uncheck all the children
        for (int i = 0; i < item->childCount(); ++i) {
            if (item->child(i)->checkState(0) != Qt::Unchecked) {
                item->child(i)->setCheckState(0, Qt::Unchecked);
            }
        }

        // Can't uncheck the root.
        if (!parent) {
            item->setCheckState(0, Qt::PartiallyChecked);
        }
    }

    if (item->checkState(0) == Qt::PartiallyChecked) {
        QTreeWidgetItem *parent = item->parent();
        if (parent && parent->checkState(0) != Qt::PartiallyChecked) {
            parent->setCheckState(0, Qt::PartiallyChecked);
        }
    }
}

QSet<QString> SelectiveSyncWidget::createBlackList(QTreeWidgetItem *root) const
{
    if (!root) {
        root = _folderTree->topLevelItem(0);
    }
    if (!root)
        return {};

    switch (root->checkState(0)) {
    case Qt::Unchecked:
        return {root->data(0, Qt::UserRole).toString() + QLatin1Char('/')};
    case Qt::Checked:
        return {};
    case Qt::PartiallyChecked:
        break;
    }

    QSet<QString> result;
    if (root->childCount()) {
        for (int i = 0; i < root->childCount(); ++i) {
            result += createBlackList(root->child(i));
        }
    } else {
        // We did not load from the server so we re-use the one from the old black list
        QString path = root->data(0, Qt::UserRole).toString();
        for (const auto &it : _oldBlackList) {
            if (it.startsWith(path))
                result += it;
        }
    }
    return result;
}

qint64 SelectiveSyncWidget::estimatedSize(QTreeWidgetItem *root)
{
    if (!root) {
        root = _folderTree->topLevelItem(0);
    }
    if (!root)
        return -1;


    switch (root->checkState(0)) {
    case Qt::Unchecked:
        return 0;
    case Qt::Checked:
        return root->data(1, Qt::UserRole).toLongLong();
    case Qt::PartiallyChecked:
        break;
    }

    qint64 result = 0;
    if (root->childCount()) {
        for (int i = 0; i < root->childCount(); ++i) {
            auto r = estimatedSize(root->child(i));
            if (r < 0)
                return r;
            result += r;
        }
    } else {
        // We did not load from the server so we have no idea how much we will sync from this branch
        return -1;
    }
    return result;
}

void SelectiveSyncWidget::setDavUrl(const QUrl &davUrl)
{
    _davUrl = davUrl;
}

QUrl SelectiveSyncWidget::davUrl() const
{
    Q_ASSERT(!_davUrl.isEmpty());
    return _davUrl;
}
}
