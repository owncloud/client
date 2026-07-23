/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "gui/spaces/spacesmodel.h"

#include <QWizardPage>


namespace Ui {
class SpacesPage;
}

namespace OCC {

namespace GraphApi {
    class SpacesManager;
}

class SpacesPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit SpacesPage(GraphApi::SpacesManager *spacesMgr, QWidget *parent);
    ~SpacesPage();

    bool isComplete() const override;


    GraphApi::Space *currentSpace() const;

private:
    Ui::SpacesPage *ui;
};

}
