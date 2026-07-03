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

#include "common/pinstate.h"

#include <QLoggingCategory>
#include <QString>
#include <QUrl>
#include <QWidget>

namespace OCC {

Q_DECLARE_LOGGING_CATEGORY(lcGuiUtility)

namespace Utility {

    /** Open a URL in the browser.
     *
     * If launching the browser fails, display a message.
     */
    bool openBrowser(const QUrl &url, QWidget *errorWidgetParent);

    /** Start composing a new email message.
     *
     * If launching the email program fails, display a message.
     */
    bool openEmailComposer(const QString &subject, const QString &body,
        QWidget *errorWidgetParent);

    /** Translated text for "making items always available locally" */
    QString vfsPinActionText();

    /** Translated text for "free up local space" (and unpinning the item) */
    QString vfsFreeSpaceActionText();

    void startShellIntegration();

    QString socketApiSocketPath();

    OWNCLOUDGUI_EXPORT void markDirectoryAsSyncRoot(const QString &path, const QUuid &accountUuid);
    std::pair<QString, QUuid> getDirectorySyncRootMarkings(const QString &path);
    void unmarkDirectoryAsSyncRoot(const QString &path);
} // namespace Utility
} // namespace OCC
