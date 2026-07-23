/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "gui/owncloudguilib.h"

#include "common/fixedsizeringbuffer.h"
#include "protocolitem.h"

#include <QAbstractTableModel>


namespace OCC {

class OWNCLOUDGUI_EXPORT ProtocolItemModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum class ProtocolItemRole {
        Action,
        File,
        Folder,
        Size,
        Account,
        Time,
        Status,

        ColumnCount
    };
    Q_ENUM(ProtocolItemRole)

    /**
     * @brief ProtocolItemModel
     * @param parent
     * @param issueMode Whether we are tracking all synced items or issues
     */
    ProtocolItemModel(size_t size, bool issueMode, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void addProtocolItem(ProtocolItem &&item);
    const ProtocolItem &protocolItem(const QModelIndex &index) const;

    bool isModelFull() const
    {
        return _data.isFull();
    }

    /**
     * Return underlying unordered raw data
     */
    auto rawData() const
    {
        return _data;
    }

    void reset(std::vector<ProtocolItem> &&data);

    void remove_if(const std::function<bool(const ProtocolItem &)> &filter);

private:
    FixedSizeRingBuffer<ProtocolItem> _data;
    bool _issueMode;
    int _maxLogSize;

};

}
