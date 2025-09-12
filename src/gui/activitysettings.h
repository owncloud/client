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

#include <QWidget>

class QTabWidget;

namespace OCC {

class ProtocolWidget;
class IssuesWidget;

/**
 * @brief The ActivitySettings class
 * @ingroup gui
 *
 * Implements a tab for the settings dialog, displaying lists of local activities and sync issues
 */
class ActivitySettings : public QWidget
{
    Q_OBJECT
public:
    explicit ActivitySettings(QWidget *parent = nullptr);
    ~ActivitySettings() override;

private Q_SLOTS:
    void slotShowIssueItemCount(int cnt);

private:
    QTabWidget *_tab = nullptr;
    int _syncIssueTabId = -1;
    int _localActivityTabId = -1;

    ProtocolWidget *_protocolWidget = nullptr;
    IssuesWidget *_issuesWidget = nullptr;
};
}
