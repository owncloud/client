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

#include <QWidget>

namespace OCC {
class AccountModalWidget;

namespace Ui {
    class AccountView;
}

class FolderMan;

class Account;
class AccountState;
class AccountFoldersView;

/**
 * @brief The AccountView class
 * @ingroup gui
 */
class OWNCLOUDGUI_EXPORT AccountView : public QWidget
{
    Q_OBJECT

public:
    explicit AccountView(QWidget *parent);
    ~AccountView() override;

    void setAccountMenuActions(QList<QAction *> actions);
    void setConnectionLabel(const QString &message, const QIcon &icon, QStringList errors = QStringList());

    // this is primarily used to run an account "modal" widget
    void setTopStackWidget(QWidget *widget);
    void removeStackWidget(QWidget *widget);

    // holding my nose for now - this should not be public, nor should the type be "embedded" in the view's ui.
    // todo: replace the concrete folders view with a placeholder location so the controller can SET the folders view in the
    // account view.
    // open question: would this mess up the squish tests? we'll soon learn the answer
    AccountFoldersView *foldersView();
    void accountSettingUpChanged(bool settingUp);


signals:
    void requestMenuActionUpdate();

protected:
    void showEvent(QShowEvent *ev) override;

private:
    Ui::AccountView *_ui;
};

} // namespace OCC

Q_DECLARE_METATYPE(OCC::AccountView)
