/*
 * Copyright (C) by Duncan Mac-Vicar P. <duncan@kde.org>
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

#include "gui/owncloudguilib.h"

#include "folderman.h"
#include "owncloudgui.h"
#include "platform.h"

#include <QPointer>

class QMessageBox;
class QSystemTrayIcon;
class QSocket;

namespace CrashReporter {
class Handler;
}

namespace OCC {

Q_DECLARE_LOGGING_CATEGORY(lcApplication)

class Theme;
class Folder;

/**
 * @brief The Application class
 * @ingroup gui
 */
class OWNCLOUDGUI_EXPORT Application : public QObject
{
    Q_OBJECT
public:
    static std::unique_ptr<Application> createInstance(Platform *platform, const QString &displayLanguage, bool debugMode);
    ~Application();

    bool debugMode();

    ownCloudGui *gui() const;

    QString displayLanguage() const;

    AccountStatePtr addNewAccount(AccountPtr newAccount);

protected Q_SLOTS:
    void slotUseMonoIconsChanged(bool);
    void slotCleanup();
    void slotAccountStateAdded(AccountStatePtr accountState) const;
    void slotAccountStateRemoved() const;

private:
    explicit Application(Platform *platform, const QString &displayLanguage, bool debugMode);

    QPointer<ownCloudGui> _gui = {};

    const bool _debugMode = false;
    QString _displayLanguage;

    static Application *_instance;
    friend Application *ocApp();
};

inline Application *ocApp()
{
    OC_ENFORCE(Application::_instance);
    return Application::_instance;
}

} // namespace OCC
