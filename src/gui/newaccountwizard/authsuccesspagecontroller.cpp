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
#include "authsuccesspagecontroller.h"

#include <QWizardPage>
namespace OCC {


AuthSuccessPageController::AuthSuccessPageController(QWizardPage *page, QObject *parent)
    : QObject{parent}
    , _page(page)
{
    buildPage();
}

void AuthSuccessPageController::buildPage()
{
    if (!_page)
        return;

    _page->setTitle(QStringLiteral("SuccessPage"));
}

bool AuthSuccessPageController::validate()
{
    return true;
}
}
