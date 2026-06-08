#pragma once

#include <QObject>
#include <QPointer>

// #include "accountmanager.h"
// #include "mainwindow/mainwindow.h"

#include <QHash>
#include <QUuid>

class QToolBar;
class QAction;
class QActionGroup;

namespace OCC {

class AccountState;
class AccountManager;
class Account;
class MainWindow;

class AccountsGuiController : public QObject
{
    Q_OBJECT

public:
    AccountsGuiController(AccountManager *accountMgr, MainWindow *window, QObject *parent);
    void runAccountWizard();


private:
    QPointer<AccountManager> _accountMgr;
    QPointer<MainWindow> _window;
    QHash<QUuid, QAction *> _actionForAccount;

    void onAccountAdded(AccountState *state);
    void onAccountRemoved(AccountState *state);
    void onAccountAvatarChanged();
    void handleAccountSetupError(const QString &error);
    void runFolderWizard(Account *account);
    void startModal(QUuid accountId);
    void endModal(QUuid accountId);
    void setupAccountPlaceholder();
    void removeAccountPlaceholder();
    void onLastAccountRemoved();
};
}
