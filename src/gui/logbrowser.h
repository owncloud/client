/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "gui/owncloudguilib.h"

#include <QDialog>

namespace OCC {

namespace Ui {
    class LogBrowser;
};

/**
 * @brief The LogBrowser class
 * @ingroup gui
 */
class OWNCLOUDGUI_EXPORT LogBrowser : public QDialog
{
    Q_OBJECT
public:
    explicit LogBrowser(QWidget *parent);
    ~LogBrowser() override;

    /** Sets Logger settings depending on ConfigFile values.
     *
     * Currently used for establishing logging to a temporary directory.
     * Will only enable logging if it isn't enabled already.
     */
    static void setupLoggingFromConfig();

protected Q_SLOTS:
    void togglePermanentLogging(bool enabled);

private:
    QScopedPointer<Ui::LogBrowser> ui;
};

} // namespace
