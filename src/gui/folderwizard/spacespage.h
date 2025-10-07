/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
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
