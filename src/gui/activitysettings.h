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

#pragma once

#include "progressdispatcher.h"
#include "models/models.h"

class QTabWidget;

namespace OCC {

class ProtocolWidget;
class IssuesWidget;

/**
 * @brief The ActivitySettings class
 * @ingroup gui
 *
 * Implements a tab for the settings dialog, displaying the three activity
 * lists.
 */
class ActivitySettings : public QWidget
{
    Q_OBJECT
public:
    explicit ActivitySettings(QWidget *parent = nullptr);
    ~ActivitySettings() override;

public Q_SLOTS:
    void slotShowIssuesTab();

private Q_SLOTS:
    void slotShowIssueItemCount(int cnt);

private:
    QTabWidget *_tab;
    int _syncIssueTabId;

    ProtocolWidget *_protocolWidget;
    IssuesWidget *_issuesWidget;
};
}
