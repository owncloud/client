/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QModelIndexList>
#include <QSortFilterProxyModel>
#include <QString>
#include <QtGlobal>

class QSortFilterProxyModel;
class QMenu;

namespace OCC {

namespace Models {
    Q_NAMESPACE

    enum DataRoles {
        UnderlyingDataRole = Qt::UserRole + 100,
        StringFormatWidthRole, // The width for a cvs formatted column

        // data() should return boolean values for this role to work in conjunction with FilteringProxyModel
        FilterRole,
    };
    Q_ENUM_NS(DataRoles)

    class SignalledQSortFilterProxyModel : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:
        using QSortFilterProxyModel::QSortFilterProxyModel;

        void setFilterFixedStringSignalled(const QString &pattern);

    Q_SIGNALS:
        void filterChanged();
    };


    /**
     * Returns a cvs representation of a table
     */
    QString formatSelection(const QModelIndexList &items, int dataRole = Qt::DisplayRole);

    std::function<void()> addFilterMenuItems(QMenu *menu, const QStringList &candidates, SignalledQSortFilterProxyModel *model, int column, const QString &columnName, int role);

    /**
     * Returns a vector with indices
     * This is handy to iterate over the columns
     */
    template <typename T>
    auto range(T start, T end)
    {
        std::vector<T> out;
        out.reserve(end - start);
        for (auto i = start; i < end; ++i) {
            out.push_back(i);
        }
        return out;
    }

    template <typename T>
    auto range(T end)
    {
        return range<T>(0, end);
    }

    class FilteringProxyModel : public QSortFilterProxyModel
    {
        using QSortFilterProxyModel::QSortFilterProxyModel;

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    };
} // OCC::Models namespace
} // OCC namespace
