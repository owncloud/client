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


#include "activitywidget.h"
#include "issueswidget.h"
#include "protocolwidget.h"

#include <QHBoxLayout>
#include <QTabWidget>


namespace OCC {

ActivitySettings::ActivitySettings(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *hbox = new QHBoxLayout(this);

    // create a tab widget for the three activity views
    _tab = new QTabWidget(this);
    hbox->addWidget(_tab);
    setLayout(hbox);

    // todo: rename ProtocolWidget to, oh I don't know...LocalActivityWidget?
    _protocolWidget = new ProtocolWidget(this);
    _localActivityTabId = _tab->addTab(_protocolWidget, Resources::getCoreIcon(QStringLiteral("states/sync")), tr("Local Activity"));

    _issuesWidget = new IssuesWidget(this);
    _syncIssueTabId = _tab->addTab(_issuesWidget, Resources::getCoreIcon(QStringLiteral("states/warning")), QString());
    slotShowIssueItemCount(0); // to display the label.
    connect(_issuesWidget, &IssuesWidget::issueCountUpdated, this, &ActivitySettings::slotShowIssueItemCount);

    // We want the local activity tab to be the default
    _tab->setCurrentIndex(_localActivityTabId);
}

void ActivitySettings::slotShowIssueItemCount(int cnt)
{
    QString cntText = tr("Not Synced");
    if (cnt) {
        //: %1 is the number of not synced files.
        cntText = tr("Not Synced (%1)").arg(cnt);
    }
    _tab->setTabText(_syncIssueTabId, cntText);
}

ActivitySettings::~ActivitySettings()
{
}
}
