#include "accountsguicontroller.h"

#include <QAction>
#include <QActionGroup>
#include <QMessageBox>
#include <QToolBar>

#include "accountmanager.h"
#include "accountplaceholderwidget.h"
#include "accountstate.h"
#include "accountview.h"
#include "creds/abstractcredentials.h"
#include "folderman.h"
#include "mainwindow/mainwindow.h"
#include "newaccountwizard/newaccountbuilder.h"
#include "newaccountwizard/newaccountwizard.h"
#include "newaccountwizard/newaccountwizardcontroller.h"
#include "resources.h"

namespace OCC {

AccountsGuiController::AccountsGuiController(MainWindow *window, QObject *parent)
    : QObject(parent)
    , _window(window)
{
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
    connect(AccountManager::instance(), &AccountManager::lastAccountRemoved, this, &AccountsGuiController::onLastAccountRemoved);

    if (accounts.isEmpty()) {
        // this is a genuinely reasonable use of timer to execute an operation after construction and any
        // other event loop actions have finished
        QTimer::singleShot(0, this, &AccountsGuiController::runAccountWizard);
    }
}

void AccountsGuiController::onAccountAdded(AccountState *state)
{
    if (!state || !state->account())
        return;
    // asap we need to create some kind of accountView builder that will instantiate a controller and view + whatever else
    // as currently everything is in the view which is absolutely not ok, especially given the multitude of "heavy lifting"
    // that goes on in there

    Account *account = state->account();
    QUuid accountId = account->uuid();
    connect(account, &Account::avatarChanged, this, &AccountsGuiController::onAccountAvatarChanged);

    auto accountView = new AccountView(state, nullptr);
    // for both the view and the action, we create a unique objectName using the account uuid
    // to support squish test object identification
    accountView->setObjectName(QString("accountView_%1").arg(accountId.toString()));

    connect(account->credentials(), &AbstractCredentials::requestAccountModal, accountView, &AccountView::onRequestAccountModalWidget);
    connect(accountView, &AccountView::accountBeginModal, this, &AccountsGuiController::startModal);
    connect(accountView, &AccountView::accountEndModal, this, &AccountsGuiController::endModal);

    QAction *accountAction = new QAction(this);
    accountAction->setObjectName(QString("accountAction_%1").arg(accountId.toString()));
    _actionForAccount.insert(accountId, accountAction);

    accountAction->setIcon(account->avatar());
    accountAction->setText(account->davUser());
    accountAction->setData(QVariant::fromValue(accountView));
    accountAction->setCheckable(true);

    _window->addAccountAction(accountAction);

    accountAction->setChecked(true);

    if (_actionForAccount.contains(QUuid()))
        removeAccountPlaceholder();
}

void AccountsGuiController::onAccountRemoved(AccountState *state)
{
    if (!state || !state->account())
        return;

    Account *acc = state->account();
    QUuid uid = acc->uuid();
    if (_actionForAccount.contains(uid)) {
        QAction *action = _actionForAccount[uid];
        _window->removeAccountAction(action);
        _actionForAccount.remove(uid);
        action->deleteLater();

        const QList<AccountState *> accounts = AccountManager::instance()->accounts();
        if (!accounts.isEmpty()) {
            QUuid newuid = accounts.last()->account()->uuid();
            if (_actionForAccount.contains(newuid))
                _actionForAccount[newuid]->setChecked(true);
        }
    }
}

void AccountsGuiController::onLastAccountRemoved()
{
    runAccountWizard();
}

void AccountsGuiController::onAccountAvatarChanged()
{
    Account *account = qobject_cast<Account *>(sender());
    if (!account)
        return;
    QAction *action = _actionForAccount.value(account->uuid(), nullptr);

    if (action) {
        action->setIcon(account->avatar());
    }
}

void AccountsGuiController::runAccountWizard()
{
    NewAccountWizard wizard(_window);
    NewAccountModel model(nullptr);
    NewAccountWizardController wizardController(&model, &wizard, nullptr);
    ownCloudGui::raise();
    int result = wizard.exec();
    if (result == QDialog::Accepted) {
        // the builder needs to be a pointer as it has to wait for the connection state to go to connected
        // it will delete itself once it has completed its mission.
        // pass this as parent only as a safeguard.
        if (!model.isComplete()) {
            QMessageBox::warning(
                _window, tr("New account failure"), tr("The information required to create a new account is incomplete. Please run the wizard again."));
        } else {
            NewAccountBuilder *builder = new NewAccountBuilder(model, this);
            FolderMan *folderman = FolderMan::instance();
            connect(builder, &NewAccountBuilder::requestSetUpSyncFoldersForAccount, folderman, &FolderMan::setUpInitialSyncFolders);
            connect(builder, &NewAccountBuilder::requestLoadSpacesOnly, folderman, &FolderMan::setUpInitialSpaces);
            connect(builder, &NewAccountBuilder::requestFolderWizard, this, &AccountsGuiController::runFolderWizard);
            connect(builder, &NewAccountBuilder::unableToCompleteAccountCreation, this, &AccountsGuiController::handleAccountSetupError);
            builder->buildAccount();
        }
    } else {
        // if there are no accounts, add a placeholder "page" to the main window that informs the user how to create one
        setupAccountPlaceholder();
    }
}

void AccountsGuiController::setupAccountPlaceholder()
{
    if (!AccountManager::instance()->accounts().isEmpty())
        return;

    QUuid uid;
    QAction *placeholderAccountAction = nullptr;
    if (_actionForAccount.contains(uid)) {
        placeholderAccountAction = _actionForAccount[uid];
    } else {
        placeholderAccountAction = new QAction(tr("Accounts"));
        placeholderAccountAction->setObjectName("placeholderAccountAction");
        placeholderAccountAction->setCheckable(true);
        placeholderAccountAction->setIcon(Resources::getCoreIcon("warning"));
        // use null uuid for placeholder action since there IS no account for it
        _actionForAccount.insert(QUuid(), placeholderAccountAction);
        auto placeholderWidget = new AccountPlaceholderWidget(_window);
        placeholderAccountAction->setData(QVariant::fromValue(placeholderWidget));
        _window->addAccountAction(placeholderAccountAction);
    }
    placeholderAccountAction->setChecked(true);
}

void AccountsGuiController::removeAccountPlaceholder()
{
    QUuid uid;
    if (_actionForAccount.contains(uid)) {
        QAction *action = _actionForAccount[uid];
        _window->removeAccountAction(action);
        _actionForAccount.remove(uid);
        action->deleteLater();
    }
}

void AccountsGuiController::handleAccountSetupError(const QString &error)
{
    QMessageBox::warning(_window, tr("New account failure"),
        tr("The account could not be created due to an error:\n%1\nPlease check the server's availability then run the wizard again.").arg(error));

    // note that the current behavior for the setup failure is that the account wizard auto-pops again because the
    // accountState for the new account was already created and it is subsequently deleted by the account builder if the final
    // setup fails for some reason. The accountRemoved handler then runs the wizard again if there are no other accounts.
    // In case we change that in future, this will create the dummy account page if the wizard doesn't auto-run again
    setupAccountPlaceholder();
}

void AccountsGuiController::runFolderWizard(Account *account)
{
    if (!account)
        return;

    QAction *action = _actionForAccount.value(account->uuid(), nullptr);
    if (!action)
        return;

    action->setChecked(true);
    AccountView *view = action->data().value<AccountView *>();
    if (view)
        view->slotAddFolder();
}

void AccountsGuiController::startModal(QUuid accountId)
{
    QAction *action = _actionForAccount.value(accountId, nullptr);
    if (!action)
        return;

    action->setIcon(Resources::getCoreIcon("states/warning"));
    action->setChecked(true);
    ownCloudGui::raise();
}

void AccountsGuiController::endModal(QUuid accountId)
{
    QAction *action = _actionForAccount.value(accountId, nullptr);
    if (!action)
        return;
    // get the icon from the account again and put it back on the action
    // action->setIcon(account->avatar());
}
}
