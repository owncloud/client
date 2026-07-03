/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef UPDATER_H
#define UPDATER_H

#include "gui/owncloudguilib.h"

#include <QLoggingCategory>
#include <QObject>

class QUrl;
class QUrlQuery;

namespace OCC {

Q_DECLARE_LOGGING_CATEGORY(lcUpdater)

class OWNCLOUDGUI_EXPORT Updater : public QObject
{
    Q_OBJECT
public:
    static Updater *instance();
    static QUrl updateUrl();

    virtual void checkForUpdate() = 0;
    virtual void backgroundCheckForUpdate() = 0;
    virtual void validateUpdate();

    /***
     * This function will just restart on most platforms
    */
    virtual void applyUpdateAndRestart();

protected:
    Updater()
        : QObject(nullptr)
    {
    }

private:
    static QUrlQuery getQueryParams();
    static Updater *create();
    static Updater *_instance;
};

} // namespace OCC

#endif // UPDATER_H
