/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "activitysettings.h"
#include "localactivitywidget.h"
#include "resources.h"
#include "syncerrorwidget.h"

#include <QHBoxLayout>
#include <QTabWidget>


namespace OCC {
ActivitySettings::ActivitySettings(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *hbox = new QHBoxLayout(this);

    // create a tab widget for the activity views
    _tab = new QTabWidget(this);
    hbox->addWidget(_tab);
    setLayout(hbox);

    auto _localActivityWidget = new LocalActivityWidget(this);
    auto localActivityTabId = _tab->addTab(_localActivityWidget, Resources::getCoreIcon(QStringLiteral("states/sync")), tr("Local Activity"));

    auto _syncErrorWidget = new SyncErrorWidget(this);
    _syncErrorTabId = _tab->addTab(_syncErrorWidget, Resources::getCoreIcon(QStringLiteral("states/warning")), QString());
    slotShowIssueItemCount(0); // to display the label.
    connect(_syncErrorWidget, &SyncErrorWidget::issueCountUpdated, this, &ActivitySettings::slotShowIssueItemCount);

    // We want the local activity tab to be the default
    _tab->setCurrentIndex(localActivityTabId);
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
    _tab->setTabText(_syncErrorTabId, cntText);
}
}