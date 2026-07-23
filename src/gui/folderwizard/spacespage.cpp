/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "spacespage.h"
#include "ui_spacespage.h"

#include "spacesmanager.h"

#include "theme.h"

using namespace OCC;

SpacesPage::SpacesPage(GraphApi::SpacesManager *spacesMgr, QWidget *parent)
    : QWizardPage(parent)
    , ui(new Ui::SpacesPage)
{
    ui->setupUi(this);

    ui->widget->setSpacesManager(spacesMgr);
    ui->label->setText(
        Theme::instance()->spacesAreCalledFolders() ? tr("Select a folder to sync it to your computer.") : tr("Select a Space to sync it to your computer."));

    connect(ui->widget, &Spaces::SpacesBrowser::currentSpaceChanged, this, &QWizardPage::completeChanged);
}

SpacesPage::~SpacesPage()
{
    delete ui;
}

bool OCC::SpacesPage::isComplete() const
{
    return ui->widget->currentSpace();
}

GraphApi::Space *OCC::SpacesPage::currentSpace() const
{
    return ui->widget->currentSpace();
}
