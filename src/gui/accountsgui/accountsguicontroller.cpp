#include "accountsguicontroller.h"

#include <QAction>
#include <QActionGroup>
#include <QMessageBox>
#include <QToolBar>

#include "accountmanager.h"
#include "accountstate.h"
#include "accountview.h"
#include "creds/abstractcredentials.h"
#include "folderman.h"
#include "mainwindow/mainwindow.h"
#include "newaccountwizard/newaccountbuilder.h"
#include "newaccountwizard/newaccountwizard.h"
#include "newaccountwizard/newaccountwizardcontroller.h"
#include "resources.h"
// this is only needed to use the pixmap function - when qml is gone move this to normal resources impl (I think)
#include "resources/qmlresources.h"


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
    connect(accountView, &AccountView::accountBeginModal, this, &AccountsGuiController::startModal);
    connect(accountView, &AccountView::accountEndModal, this, &AccountsGuiController::endModal);

    QAction *accountAction = new QAction(this);
    _actionForAccount.insert(state->account()->uuid(), accountAction);

    accountAction->setIcon(account->avatar());
    accountAction->setText(account->davUser());
    accountAction->setData(QVariant::fromValue(accountView));
    accountAction->setCheckable(true);

    _window->addAccountAction(accountAction);

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
        if (!_actionForAccount.isEmpty()) {
            const auto actions = _actionForAccount.values();
            actions.first()->setChecked(true);
            // I have tried EVERYTHING to get rid of clazy warnings about calling QList::front (which I never even call!)
            // and/or QList::first on temporary and it simply will not go away.
            // I honestly could not care less at this point. I think clazy is about as reliable as tidy at this point - it's just noise
        }
    }
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
    } else
        qDebug() << "wizard rejected";
}

void AccountsGuiController::handleAccountSetupError(const QString &error)
{
    QMessageBox::warning(_window, tr("New account failure"),
        tr("The account could not be created due to an error:\n%1\nPlease check the server's availability then run the wizard again.").arg(error));
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
