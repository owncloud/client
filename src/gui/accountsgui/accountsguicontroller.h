#pragma once

#include <QObject>

#include <QHash>
#include <QUuid>

class QToolBar;
class QAction;
class QActionGroup;

namespace OCC {

class AccountState;
class Account;
class MainWindow;

class AccountsGuiController : public QObject
{
    Q_OBJECT

public:
    AccountsGuiController(MainWindow *window, QObject *parent);
    void runAccountWizard();


private:
    MainWindow *_window = nullptr;
    QHash<QUuid, QAction *> _actionForAccount;
    QActionGroup *_actionGroup = nullptr;

    void onAccountAdded(AccountState *state);
    void onAccountRemoved(AccountState *state);
    void setCurrentAccount(Account *account);
    void onAccountAvatarChanged();
    void handleAccountSetupError(const QString &error);
};
}
