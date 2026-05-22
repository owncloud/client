#include "accountsguicontroller.h"

#include <QAction>
#include <QActionGroup>
#include <QToolBar>

#include "accountmanager.h"
#include "accountstate.h"
#include "accountview.h"
#include "credentials.h"
#include "mainwindow/mainwindow.h"


namespace OCC {

AccountsGuiController::AccountsGuiController(MainWindow *window, QObject *parent)
    : QObject(parent)
    , _window(window)
{
    // I think this should move to main window because we will need to add other actions to the group, eg the error view
    // and possibly the local activity view - if that does not go into a modal impl via the menu
    _actionGroup = new QActionGroup(this);
    _actionGroup->setExclusive(true);

    // load any existing accounts from the manager.
    const auto accounts = AccountManager::instance()->accounts();
    for (const auto &accountState : accounts) {
        onAccountAdded(accountState);
    }
    if (!accounts.isEmpty()) {
        _actionForAccount.value(accounts.first()->account()->uuid(), nullptr)->setChecked(true);
    }

    connect(AccountManager::instance(), &AccountManager::accountAdded, this, &AccountsGuiController::onAccountAdded);
    connect(AccountManager::instance(), &AccountManager::accountRemoved, this, &AccountsGuiController::onAccountRemoved);
}

void AccountsGuiController::onAccountAdded(AccountState *state)
{
    if (!state || !state->account())
        return;
    // asap we need to create some kind of accountView builder that will instantiate a controller and view + whatever else
    // as currently everything is in the view which is absolutely not ok, especially given the multitude of "heavy lifting"
    // that goes on in there

    Account *account = state->account();
    connect(account, &Account::avatarChanged, this, &AccountsGuiController::onAccountAvatarChanged);

    auto accountView = new AccountView(state, nullptr);
    connect(account->credentials(), &AbstractCredentials::requestAccountModal, accountView, &AccountView::onRequestAccountModalWidget);
    connect(accountView, &AccountView::accountBeginModal, _window, &MainWindow::startModal);
    connect(accountView, &AccountView::accountEndModal, _window, &MainWindow::stopModal);

    QAction *accountAction = new QAction(this);
    _actionForAccount.insert(state->account()->uuid(), accountAction);
    accountAction->setIcon(account->avatar());
    accountAction->setText(account->davUser());
    accountAction->setData(QVariant::fromValue(accountView));
    accountAction->setCheckable(true);

    _actionGroup->addAction(accountAction);
    _window->addAccountAction(accountAction);

    // this should autoselect the page from the data() value
    accountAction->setChecked(true);
}

void AccountsGuiController::onAccountRemoved(AccountState *state)
{
    if (!state || !state->account())
        return;
    // todo: #37. using the account after we know it's been removed is not ok.
    Account *acc = state->account();

    if (QAction *action = _actionForAccount.value(acc->uuid(), nullptr)) {
        _window->removeAccountAction(action);
        _actionForAccount.remove(acc->uuid());
        action->deleteLater();
        // just select something from the account actions - I don't think it really matters what but can adjust
        // if people complain
        if (!_actionForAccount.isEmpty())
            _actionForAccount.values().front()->setChecked(true);
    }
}

void AccountsGuiController::onAccountAvatarChanged()
{
    Account *account = qobject_cast<Account *>(sender());
    if (!account)
        return;
    QAction *action = _actionForAccount.value(account->uuid(), nullptr);
    if (action)
        action->setIcon(account->avatar());
}

void AccountsGuiController::setCurrentAccount(Account *account)
{
    // if (!account || account == _currentAccount)
    //    return;

    //_currentAccount = account;

    //_ui->stack->setCurrentWidget(accountView(account));
    //_currentPage = SettingsPage::Account;

    // Q_EMIT currentAccountChanged();
    // Q_EMIT currentPageChanged();
}
}
