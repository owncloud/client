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

#include <QDebug>

namespace OCC {


AccountFoldersView::AccountFoldersView(QWidget *parent)
    : QWidget{parent}
{
    // important to know: the parent in this ctr is always null because the widget is instantiated by the ui impl. it leaves the parent
    // null, presumably because it adds it to a stacked widget after ctr which takes ownership as parent. The issue with this is that you *can't*
    // do anything relative to the parent in this construction setup because it is not known at time of construction.

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

    // this should allow error items to paint fully just using the default impl
    //_treeView->setWordWrap(true);
    // ha! no that does not work at all...google says it's been broken for at least 15 years.
    // nice job guys.

    _treeView->header()->setSectionsMovable(false);
    _treeView->setHeaderHidden(true);

    // this method of correcting the row selection color to match the app button highlight does not work when user has changed system settings (the color sticks
    // with the color that matched the system settings on creation of the view, which is bad. this color correction has been moved to the item delegates for now
    // because we can always grab the true current color on paint.
    // TODO: #60 - we really need to implement full palettes for both light and dark mode. my rec would be to use a QStyle sub but will
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

bool AccountFoldersView::eventFilter(QObject *obj, QEvent *ev)
{
    // we do not get clicked events here - no idea why. In addition, the treeview->clicked signal is sent so rarely it's fairly
    // useless. I have no idea how this is supposed to work as all mouse events should be reaching the tree at least? Apparently not.
    // We do get a InputMethodQueryEvent each time but I think this only relates to text based editor delegates? No idea why we get this
    // for a non-text editor
    // furthermore, we seem to get a focus event for *every* mouse triggered row selection, which strikes me as weird.
    // Anyway this filter works for making sure the current item is always in edit mode when the tree gains focus, regardless of how it happens
    qDebug() << "tree view event filter event " << ev->type();
    if (obj == _treeView) {
        // QModelIndex current = _treeView->currentIndex();
        // qDebug() << "event filter current index " << current;
        // Extremely important: we need to respond to both of these events to put the row in edit mode, otherwise direct clicking the button column in the
        // selected row pops doesn't work correctly. I have no idea why this is, but this is *very* particular, and needs to be combined with the event handling
        // in the button delegate handleEvent to ensure it all works "well". note that "enter" is mouse enter, so it may be a little bit funny to have the tree
        // go into edit mode and focus when you mouse over the tree, but will try to adapt it later if there are complaints. So far this is really the only
        // thing that fully worked. WindowActivate quasi-works, but the button may not be focused in some cases.


        // this ensures that if a tree item is in edit mode, but is currently scrolled out of view, hitting edit trigger scrolls to the item
        // before popping the button menu
        if (ev->type() == QEvent::ShortcutOverride) {
            _treeView->scrollTo(_treeView->currentIndex());
            return true;
        }

        //  id (ev->type() == )
        // if I don't include showToParent in this, direct clicking the selected item on first show is fubar - menu pops but in a very strange location
        if (ev->type() == QEvent::FocusIn || ev->type() == QEvent::ShowToParent) {
            QModelIndex current = _treeView->currentIndex();
            qDebug() << "event filter index " << current.row() << "," << current.column();
            _treeView->setCurrentIndex(current);
            _treeView->scrollTo(current);
            _treeView->edit(current);
            return true;
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
    // of other settings.
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
    // only pop the menu on folder items. TODO: work out a secondary menu to allow copy of any sync error(s)
    // shown in the child items
    if (!_treeView->currentIndex().parent().isValid())
        _itemMenu->exec(_treeView->viewport()->mapToGlobal(pos));
}
}
