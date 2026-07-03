/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QStandardItem>

namespace OCC {

class FolderItem;

/**
 * @brief The FolderErrorItem class will present any errors that arise on sync
 *
 * the error items will be children of the related FolderItem
 *
 */
class FolderErrorItem : public QStandardItem
{
    FolderErrorItem(FolderItem *parent);

private:
    FolderItem *_parent;
};
}
