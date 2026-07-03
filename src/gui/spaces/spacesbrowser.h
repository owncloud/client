/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once
#include "gui/qmlutils.h"

#include "libsync/account.h"
#include "libsync/graphapi/space.h"

#include <QSortFilterProxyModel>
#include <QWidget>

namespace Ui {
class SpacesBrowser;
}

namespace OCC::Spaces {
class SpacesModel;

class SpacesBrowser : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QSortFilterProxyModel *model MEMBER _sortModel READ model CONSTANT)
    Q_PROPERTY(GraphApi::Space *currentSpace MEMBER _currentSpace READ currentSpace NOTIFY currentSpaceChanged)
    QML_ELEMENT
    QML_UNCREATABLE("C++ only")
    OC_DECLARE_WIDGET_FOCUS
public:
    explicit SpacesBrowser(QWidget *parent = nullptr);
    ~SpacesBrowser();

    void setSpacesManager(OCC::GraphApi::SpacesManager *spacesMgr);

    GraphApi::Space *currentSpace();

    QSortFilterProxyModel *model();

Q_SIGNALS:
    void currentSpaceChanged(GraphApi::Space *space);

private:
    ::Ui::SpacesBrowser *ui;

    SpacesModel *_model;
    QSortFilterProxyModel *_sortModel;
    GraphApi::Space *_currentSpace = nullptr;
};

}
