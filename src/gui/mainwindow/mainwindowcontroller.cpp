#include "mainwindowcontroller.h"

#include "aboutview.h"
#include "accountmanager.h"
#include "accountsgui/accountsguicontroller.h"
#include "localactivitywidget.h"
#include "mainwindow.h"
#include "modalwrapperwidget.h"
#include "settingsview.h"
#include "syncerrorwidget.h"
#include "theme.h"

#include <QAction>
#include <QMessageBox>
#include <QTimer>

namespace OCC {

MainWindowController::MainWindowController(MainWindow *window, QObject *parent)
    : QObject{parent}
    , _window(window)
{
    setup();
}

void MainWindowController::setup()
{
    createSyncErrorsAction();
    createActivityAction();
    buildMenuActions();

    _accountsController = new AccountsGuiController(AccountManager::instance(), _window, this);
}

void MainWindowController::buildMenuActions()
{
    QList<QAction *> menuActions;

    QAction *addAccountAction = new QAction(tr("Add account..."), this);
    addAccountAction->setObjectName("addAcountAction");
    connect(addAccountAction, &QAction::triggered, this, &MainWindowController::onAddAccount);
    menuActions.push_back(addAccountAction);

    QAction *settingsAction = new QAction(tr("Settings..."), this);
    settingsAction->setObjectName("settingsAction");
    connect(settingsAction, &QAction::triggered, this, &MainWindowController::onSettings);
    menuActions.push_back(settingsAction);

    QAction *aboutAction = new QAction(tr("About..."), this);
    aboutAction->setObjectName("aboutAction");
    connect(aboutAction, &QAction::triggered, this, &MainWindowController::onAbout);
    menuActions.push_back(aboutAction);

    QAction *separator = new QAction(this);
    separator->setObjectName("sparatorAction");
    separator->setSeparator(true);
    menuActions.push_back(separator);

    QAction *quitAction = new QAction(tr("Quit"), this);
    quitAction->setObjectName("quitAction");
    connect(quitAction, &QAction::triggered, this, &MainWindowController::onQuit);
    menuActions.push_back(quitAction);
    _window->setMoreMenuActions(menuActions);
}

void MainWindowController::createSyncErrorsAction()
{
    QAction *syncErrorsAction = new QAction(tr("Errors: %1").arg(0), this);
    syncErrorsAction->setIcon(Resources::getCoreIcon("states/error"));
    syncErrorsAction->setObjectName("syncErrorsAction");
    syncErrorsAction->setCheckable(true);
    auto syncErrorWidget = new SyncErrorWidget(_window);
    connect(
        syncErrorWidget, &SyncErrorWidget::issueCountUpdated, this, [syncErrorsAction](int count) { syncErrorsAction->setText(tr("Errors: %1").arg(count)); });

    syncErrorsAction->setData(QVariant::fromValue(syncErrorWidget));
    _window->addGeneralAction(syncErrorsAction);
}

void MainWindowController::createActivityAction()
{
    QAction *activityAction = new QAction(tr("Activity"), this);
    activityAction->setIcon(Resources::getCoreIcon("states/sync"));
    activityAction->setObjectName("activityAction");
    activityAction->setCheckable(true);
    auto localActivityWidget = new LocalActivityWidget(_window);
    activityAction->setData(QVariant::fromValue(localActivityWidget));
    _window->addGeneralAction(activityAction);
}

void MainWindowController::onAddAccount()
{
    _accountsController->runAccountWizard();
}

void MainWindowController::onSettings()
{
    SettingsView *settings = new SettingsView(_window);
    ModalWrapperWidget *wrapper = new ModalWrapperWidget(settings, _window);
    _window->showModalWidget(wrapper);
}

void MainWindowController::onAbout()
{
    AboutView *aboutPanel = new AboutView(_window);
    ModalWrapperWidget *wrapper = new ModalWrapperWidget(aboutPanel, _window);
    _window->showModalWidget(wrapper);
}

void MainWindowController::onQuit()
{
    // this is just copied/pasted from the original - most likely will change this to a simple dialog exec, then we
    // don't need any delayed handling of the actual call to app quit
    auto box = new QMessageBox(QMessageBox::Question, tr("Quit %1").arg(Theme::instance()->appNameGUI()),
        tr("Are you sure you want to quit %1?").arg(Theme::instance()->appNameGUI()), QMessageBox::Yes | QMessageBox::No, _window);
    box->setAttribute(Qt::WA_DeleteOnClose);
    connect(box, &QMessageBox::accepted, this, [] {
        // delay quit to prevent a Qt 6.6 crash in the destructor of the dialog
        QTimer::singleShot(0, qApp, &QCoreApplication::quit);
    });
    box->open();
}
}
