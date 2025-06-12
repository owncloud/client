/*
 * Copyright (C) Lisa Reese <lisa.reese@kiteworks.com>
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
#include "urlpagecontroller.h"

#include "accessmanager.h"
#include <QWizardPage>

namespace OCC {

UrlPageController::UrlPageController(QWizardPage *page, AccessManager *accessManager, QObject *parent)
    : QObject(parent)
    , _page(page)
    , _accessManager(accessManager)
{
}

bool UrlPageController::validate()
{
    // do all the stuff
    // if it works, tell the main controller and provide the results. tbd whether we have a special data model or not.
    // so far I don't think the main controller needs to know about failures since we can set the error on the page directly.
    // obvs that needs to be reality checked.
    Q_EMIT success(QUrl(), QUrl(), {});
    return true;
    // simply return false if it fails.
    // Basically we should be able to rely on the QWizard not advancing the page if current validation fails.
}

}
