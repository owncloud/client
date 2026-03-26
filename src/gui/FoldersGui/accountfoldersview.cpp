/*
 * Copyright (C) Lisa Reese <lisa.reese@kiteworks.com>
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

#include "accountfoldersview.h"

#include <QApplication>
#include <QFocusEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>

#include "buttondelegate.h"
#include "commonstrings.h"
#include "folderitemdelegate.h"

namespace OCC {


AccountFoldersView::AccountFoldersView(QWidget *parent)
    : QWidget{parent}
{
    // important to know: the parent in this ctr is always null because the widget is instantiated by the ui impl. it leaves the parent
    // null, presumably because it adds it to a stacked widget after ctr which takes ownership as parent. The issue with this is that you *can't*
    // do anything relative to the parent in this construction setup because it is not known at time of construction.

    // possibly more important, we can't pass the model(s) via the ctr which is particularly unpleasant, because we have to declare a public interface
    // do set them after ctr. The issue with this is that the models should be immutable once set, which is most cleanly accomplished by passing the
    // deps via the ctr, NOT a setter.

    _itemMenu = new QMenu(this);
    _itemMenu->setObjectName("folderOptionsMenu");
    _itemMenu->setAccessibleName(tr("Sync options menu"));

    buildView();
}

void AccountFoldersView::buildView()
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    QString titleString = tr("%1 sync").arg(CommonStrings::capSpace());
    QLabel *titleLabel = new QLabel(titleString, this);
    QFont f = titleLabel->font();
    f.setBold(true);
    titleLabel->setFont(f);
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLayout->addWidget(titleLabel, 0, Qt::AlignLeft);

    QHBoxLayout *buttonLineLayout = new QHBoxLayout();
    QString descString = tr("Manage your synced %1.").arg(CommonStrings::spaces());
    QLabel *description = new QLabel(descString, this);
    description->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    buttonLineLayout->addWidget(description, 0, Qt::AlignLeft);

    QString addSyncString = tr("Add new %1 sync…").arg(CommonStrings::space());
    _addFolderButton = new QPushButton(addSyncString, this);
    _addFolderButton->setObjectName("addFolderSyncButton");
    _addFolderButton->setFocusPolicy(Qt::StrongFocus);
    connect(_addFolderButton, &QPushButton::clicked, this, &AccountFoldersView::addFolderTriggered);
    buttonLineLayout->addStretch(1);
    buttonLineLayout->addWidget(_addFolderButton, 0, Qt::AlignRight);
    mainLayout->addLayout(buttonLineLayout);

    _treeView = new QTreeView(this);
    _treeView->setObjectName("accountFoldersTreeView");
    _treeView->setAccessibleName(tr("%1 list view").arg(CommonStrings::space()));
    _treeView->setAccessibleDescription(tr("Navigate the %1 list using the up and down arrows").arg(CommonStrings::space()));
    _treeView->setFocusPolicy(Qt::StrongFocus);
    _treeView->installEventFilter(this);

    _treeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // indentation is "required" to show the expanders on folder when there are errors
    // I played around with this a lot and I think letting the default indent ride is the best choice, otherwise it can be hard
    // to trigger the expander consistently, among other things
    //_treeView->setIndentation(10);

    _treeView->setItemsExpandable(true);
    _treeView->setExpandsOnDoubleClick(true);
    _treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    _treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    _treeView->setAllColumnsShowFocus(true);

    // this should allow error items to paint fully on multiple lines just using the default impl
    //_treeView->setWordWrap(true);
    // ha! no that does not work at all...google says it's been broken for at least 15 years.
    // nice job guys.

    _treeView->header()->setSectionsMovable(false);
    _treeView->setHeaderHidden(true);

    // this method of correcting the row selection color to match the app button highlight does not work when user has changed system settings (the color sticks
    // with the color that matched the system settings on creation of the view, which is bad. this color correction has been moved to the item delegates for now
    // because we can always grab the true current color on paint.
    // TODO: #60 - if we really need to implement full palettes for both light and dark mode, my rec would be to use a QStyle sub but will
    // make a decision when the time comes.
    QStyleOptionButton buttonStyle;
    QPalette treePalette = _treeView->palette();
    treePalette.setColor(QPalette::Highlight, buttonStyle.palette.color(QPalette::Highlight));
    _treeView->setPalette(treePalette);


    FolderItemDelegate *delegate = new FolderItemDelegate(_treeView->indentation(), _treeView);
    _treeView->setItemDelegateForColumn(0, delegate);
    // note this is not the normal ellipses character, it's vertically centered instead of positioned at font baseline. This is better
    // for this button than normal ellipses. We also have an elipses icon (core/more.svg) but it looks quite bad in the button so text it is
    ButtonDelegate *buttonDel = new ButtonDelegate("⋯", _treeView);
    buttonDel->setMenu(_itemMenu);
    _treeView->setItemDelegateForColumn(1, buttonDel);

    _treeView->setEditTriggers(QAbstractItemView::CurrentChanged | QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);

    _treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(_treeView, &QWidget::customContextMenuRequested, this, &AccountFoldersView::popItemMenu);

    mainLayout->addWidget(_treeView);

    _syncedFolderCountLabel = new QLabel("placeholder for sync count", this);
    _syncedFolderCountLabel->setObjectName("syncedFolderCount");
    _syncedFolderCountLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLayout->addWidget(_syncedFolderCountLabel, 0, Qt::AlignLeft);

    setLayout(mainLayout);
}

// this really belongs in a tree view controller...
bool AccountFoldersView::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == _treeView) {
        QModelIndex current = _treeView->currentIndex();
        qDebug() << "event filter ev type " << ev->type();
        // this has been the only way I've found so far to ensure the FocusIn behavior works correct on first focus enter in the tree after show
        // this should never be necessary but it's unclear why exactly first focus is so broken (using tab or direct mouse click)
        if (ev->type() == QEvent::ShowToParent) {
            if (_treeView->model()->rowCount(QModelIndex()) <= 0)
                return false;
            if (!current.isValid()) {
                current = _treeView->model()->index(0, 1);
                _treeView->setCurrentIndex(current);
                qDebug() << "rows in tree on showToParent " << _treeView->model()->rowCount(QModelIndex());
            }
            // clear the selection no matter what! on first show there should be no item selected, else the automatic tree edit mode doesn't work for
            // the "default" selected item
            _treeView->setCurrentIndex(QModelIndex());
            return true;
        }

        if (ev->type() == QEvent::UpdateLater) {
            if (_treeView->model()->rowCount(QModelIndex()) <= 0)
                return false;
            if (!current.isValid()) {
                current = _treeView->model()->index(0, 1);
                _treeView->setCurrentIndex(current);
                qDebug() << "rows in tree on UpdateLater " << _treeView->model()->rowCount(QModelIndex());
            }
            // clear the selection no matter what! on first show there should be no item selected, else the automatic tree edit mode doesn't work for
            // the "default" selected item
            _treeView->setCurrentIndex(QModelIndex());
            return true;
        }
        // this ensures that if a tree item is in edit mode, but is currently scrolled out of view, hitting edit trigger scrolls to the item
        // before popping the button menu
        if (ev->type() == QEvent::ShortcutOverride) {
            _treeView->scrollTo(current);
            return true;
        }

        // if I don't include showToParent in this, direct clicking the selected item on first show is fubar - menu pops but in a very strange location
        if (ev->type() == QEvent::FocusIn) {
            QFocusEvent *focus = dynamic_cast<QFocusEvent *>(ev);
            if (!focus || focus->lostFocus())
                return false;
            //   qDebug() << "focus reason " << focus->reason();
            // this covers the case where the user selects an item in the tree by keyboard or mouse for the very first time - see the prerequisite handling
            // of QEvent::ShowToParent which is needed to make this work, else the tree is unable to correctly edit the default selected item and
            // behaves strangely.
            if (!current.isValid() && focus->reason() == Qt::TabFocusReason) {
                current = _treeView->model()->index(0, 1);
                _treeView->setCurrentIndex(current);
                if (current.flags() & Qt::ItemIsEditable)
                    _treeView->edit(current);
                return true;
            }
            if (focus->reason() == Qt::MouseFocusReason) {
                qDebug() << "mouseFocusReason current = " << current.row() << "," << current.column();
                if (current.flags().testFlag(Qt::ItemIsEditable)) {
                    _treeView->edit(current);
                    return true;
                } else
                    _treeView->edit(QModelIndex());
                return false;
            }
            if (focus->reason() == Qt::ActiveWindowFocusReason || focus->reason() == Qt::TabFocusReason) {
                qDebug() << "focus current item on active window or Tab change: " << current.row() << "," << current.column();
                _treeView->scrollTo(current);
                if (current.flags() & Qt::ItemIsEditable)
                    _treeView->edit(current);
                return true;
            }
        }
    }
    return false;
}

void AccountFoldersView::setItemModels(QStandardItemModel *model, QItemSelectionModel *selectionModel)
{
    // order here is important, if you set the selection model before the main model the selection model for the view
    // reverts to the "default" selectionModel.
    // also the tree view doesn't manage the lifetime of the selection  model so we delete the default here
    // and no we can't just use the default selection model directly as it doesn't exist when the model controller
    // is being set up :/ Thank you, .ui impl
    _treeView->setModel(model);
    QItemSelectionModel *origSelectionModel = _treeView->selectionModel();
    _treeView->setSelectionModel(selectionModel);
    origSelectionModel->deleteLater();

    // these settings can only work if/when the model has been added to the view because the header "data" comes from the model.
    // without the header data the logical indexes do not exist so...we can't do it in the ctr unless we inject the models there.
    // but we can't do that either as the view is currently created by the .ui impl, which can't take those params.
    QHeaderView *header = _treeView->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    // ResizeToContents is required to allow the button delegate size hint to work - else it defaults to 100px wide regardless
    // of other settings. On windows the button is much wider, even with this setting.
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
}

void AccountFoldersView::setSyncedFolderCount(int synced, int total)
{
    _syncedFolderCountLabel->setText(tr("%1 out of %2 %3 are synchronized").arg(QString::number(synced), QString::number(total), CommonStrings::spaces()));
}

void AccountFoldersView::enableAddFolder(bool enableAdd)
{
    _addFolderButton->setEnabled(enableAdd);
}

void AccountFoldersView::setFolderActions(QList<QAction *> actions)
{
    if (_itemMenu) {
        _itemMenu->clear();
        _itemMenu->addActions(actions);
        connect(_itemMenu, &QMenu::aboutToShow, this, &AccountFoldersView::refreshMenu);
    }
}

void AccountFoldersView::refreshMenu()
{
    emit requestActionsUpdate();
}

void AccountFoldersView::popItemMenu(const QPoint &pos)
{
    // only pop the menu on folder items. TODO: possibly work out a secondary menu to allow copy of any sync error(s)
    // shown in the child items. the old folder list had no copy function that I know of so I see this as a nice to have, not a must
    if (!_treeView->currentIndex().parent().isValid())
        _itemMenu->exec(_treeView->viewport()->mapToGlobal(pos));
}
}
