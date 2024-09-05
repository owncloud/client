/*
 * Copyright (C) Fabian Müller <fmueller@owncloud.com>
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

#include "enums.h"

#include "gui/qmlutils.h"

#include <QHBoxLayout>
#include <QMap>
#include <QRadioButton>
#include <QString>
#include <QWidget>

namespace OCC::Wizard {

/**
 * Provides a radio button based quick navigation on the wizard's bottom side.
 */
class Navigation : public QmlUtils::OCQuickWidget
{
    Q_OBJECT
    Q_PROPERTY(QList<SetupWizardState> states READ states NOTIFY statesChanged)
    Q_PROPERTY(SetupWizardState activeState READ activeState WRITE setActiveState NOTIFY activeStatesChanged)
    QML_ELEMENT
    QML_UNCREATABLE("C++")

public:
    explicit Navigation(QWidget *parent = nullptr);


    /**
     * Set or replace entries in the navigation.
     * This method creates the corresponding buttons.
     * @param newEntries ordered list of wizard states to be rendered in the navigation
     */
    void setStates(const QList<SetupWizardState> &newEntries);
    QList<SetupWizardState> states() const;

    /**
     * Change to another state. Applies changes to hosted UI elements (e.g., disables buttons, )
     */
    void setActiveState(SetupWizardState activeState);
    SetupWizardState activeState() const;

    Q_INVOKABLE QString stateDisplayName(SetupWizardState state) const;

Q_SIGNALS:
    /**
     * Emitted when a state is clicked.
     * This event is only emitted for previous states.
     * @param clickedState state the user wants to switch to
     */
    void stateClicked(SetupWizardState clickedState);
    void statesChanged();
    void activeStatesChanged();

private:
    QList<SetupWizardState> _states;
    SetupWizardState _activeState = SetupWizardState::FirstState;
    bool _enabled = true;
};

}
