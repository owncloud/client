/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "spacesbrowser.h"
#include "ui_spacesbrowser.h"

#include "spacesmodel.h"

#include "gui/accountmanager.h"
#include "gui/spaces/spaceimageprovider.h"

#include <QMenu>
#include <QQmlContext>
#include <QSortFilterProxyModel>

using namespace OCC;
using namespace OCC::Spaces;

namespace {
class SpaceFilter : public QSortFilterProxyModel
{
    using QSortFilterProxyModel::QSortFilterProxyModel;

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        auto index = sourceModel()->index(sourceRow, 0, sourceParent);
        return index.data(static_cast<int>(SpacesModel::Roles::Enabled)).toBool() && !index.data(static_cast<int>(SpacesModel::Roles::IsSynced)).toBool();
    }
};
}

SpacesBrowser::SpacesBrowser(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SpacesBrowser)
{
    ui->setupUi(this);

    _model = new SpacesModel(this);
    _sortModel = new SpaceFilter(this);
    _sortModel->setSourceModel(_model);
    _sortModel->setSortRole(static_cast<int>(SpacesModel::Roles::Priority));
    _sortModel->sort(0, Qt::DescendingOrder);

    ui->quickWidget->setOCContext(QUrl(QStringLiteral("qrc:/qt/qml/org/ownCloud/gui/spaces/qml/SpacesView.qml")), this);

    setFocusProxy(ui->quickWidget);
}

SpacesBrowser::~SpacesBrowser()
{
    delete ui;
}

void SpacesBrowser::setSpacesManager(OCC::GraphApi::SpacesManager *spacesMgr)
{
    if (spacesMgr) {
        _model->setSpacesManager(spacesMgr);
        ui->quickWidget->engine()->addImageProvider(QStringLiteral("space"), new Spaces::SpaceImageProvider(spacesMgr));
    }
}

GraphApi::Space *SpacesBrowser::currentSpace()
{
    return _currentSpace;
}

QSortFilterProxyModel *SpacesBrowser::model()
{
    return _sortModel;
}
