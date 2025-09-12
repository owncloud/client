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

#include <QtGui>
#include <QtWidgets>

#include "account.h"
#include "activitysettings.h"
#include "issueswidget.h"
#include "localactivitywidget.h"
#include "theme.h"

#include <climits>

namespace OCC {

ActivitySettings::ActivitySettings(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *hbox = new QHBoxLayout(this);
    setLayout(hbox);

    // create a tab widget for the three activity views
    _tab = new QTabWidget(this);
    hbox->addWidget(_tab);
    _localActivityWidget = new LocalActivityWidget(this);
    _tab->addTab(_localActivityWidget, Resources::getCoreIcon(QStringLiteral("states/sync")), tr("Local Activity"));

    _issuesWidget = new IssuesWidget(this);
    _syncIssueTabId = _tab->addTab(_issuesWidget, Resources::getCoreIcon(QStringLiteral("states/warning")), QString());
    slotShowIssueItemCount(0); // to display the label.
    connect(_issuesWidget, &IssuesWidget::issueCountUpdated,
        this, &ActivitySettings::slotShowIssueItemCount);

    // We want the protocol be the default
    _tab->setCurrentIndex(1);
}

ActivitySettings::~ActivitySettings()
{
}

void ActivitySettings::slotShowIssueItemCount(const int cnt)
{
    QString cntText = tr("Not Synced");
    if (cnt) {
        //: %1 is the number of not synced files.
        cntText = tr("Not Synced (%1)").arg(cnt);
    }
    _tab->setTabText(_syncIssueTabId, cntText);
}

void ActivitySettings::slotShowIssuesTab()
{
    if (_syncIssueTabId == -1)
        return;
    _tab->setCurrentIndex(_syncIssueTabId);
}
}
