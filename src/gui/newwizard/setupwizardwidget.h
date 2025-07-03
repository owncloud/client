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

#include "gui/settingsdialog.h"
#include "navigation.h"
#include "pages/abstractsetupwizardpage.h"
#include "setupwizardaccountbuilder.h"

namespace Ui {
class SetupWizardWidget;
}

namespace OCC::Wizard {

/**
 * This class contains the UI-specific code. It hides the complexity from the controller, and provides a high-level API.
 */
class SetupWizardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SetupWizardWidget(SettingsDialog *parent);
    ~SetupWizardWidget() noexcept override;

    /**
     * Set entries in the pagination at the bottom of the wizard UI.
     * The entries are identified by their position in the list (read: index).
     */
    void setNavigationEntries(const QList<SetupWizardState> &entries);

    /**
     * Render this page within the wizard window.
     * @param page page to render
     * @param index index to highlight in pagination (also used to decide which buttons to enable)
     */
    void displayPage(AbstractSetupWizardPage *page, SetupWizardState state);

    void showErrorMessage(const QString &errorMessage);

    void disableNextButton();

Q_SIGNALS:
    void navigationEntryClicked(SetupWizardState clickedState);
    void nextButtonClicked();
    void backButtonClicked();
    void rejected();

public Q_SLOTS:
    /**
     * Show "transition to next page" animation. Use displayPage(...) to end it.
     */
    void slotStartTransition();

private Q_SLOTS:
    void slotReplaceContent(QWidget *newWidget);
    void slotHideErrorMessageWidget();
    void slotMoveToNextPage();
    void slotUpdateNextButton();

private:
    void loadStylesheet();

    ::Ui::SetupWizardWidget *_ui;

    // the wizard window keeps at most one widget inside the content widget's layout
    // we keep a pointer in order to be able to delete it (and thus remove it from the window) when replacing the content
    QWidget *_currentContentWidget = nullptr;

    // need to keep track of the current page for event filtering
    AbstractSetupWizardPage *_currentPage = nullptr;
    // during a transition, the event filter must be disabled
    bool _transitioning = false;
};
}
