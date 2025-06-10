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

#pragma once

#include <QObject>


class QWizard;

class NewAccountModel;

class NewAccountWizardController : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief NewAccountWizardController
     * @param model must be non-null. Normally the model would be parented by whatever creates the triad, but in this scenario we might need to
     * take a different approach since the whole triad has a short life.
     * @param view which may be null (eg, to support testing). The creator of the view is responsible for parenting it correctly to some other gui element
     * @param parent normally this will be a pointer to whatever instantiated the controller - this is normally a manager or another controller but since the
     * triad is short lived, we may manage the controller lifetime more directly. Nevertheless, the parent should be non-null.
     */
    explicit NewAccountWizardController(NewAccountModel *model, QWizard *view, QObject *parent);

Q_SIGNALS:
    void wizardComplete();
    void wizardCanceled();

protected Q_SLOTS:
    // slots for "top level" wizard activity signals
    //  slots for model notifications if useful

    // slots that handle activity on an individual page are implemented in the associated page controller
private:
    /** builds the page controller/page widget pairs */
    void buildPages();
    /** configures the wizard with proper settings */
    void setupWizard();
    /** connects "top level" wizard signals to local slots as needed */
    void connectWizard();
    /** connects the model signals to local slots, as needed */
    void connectModel();

    NewAccountModel *_model;
    QWizard *_wizard;
};
