#pragma once

#include <QObject>
#include <QPointer>

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
    void onLastAccountRemoved();

    void onAccountAvatarChanged();

    void runFolderWizard(Account *account);
    void handleAccountSetupError(const QString &error);

    void startModal(QUuid accountId);
    void endModal(QUuid accountId);

    void setupAccountPlaceholder();
    void removeAccountPlaceholder();
};
}
