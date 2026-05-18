#include "mainwindowcontroller.h"

#include "aboutdialog.h"
#include "mainwindow.h"
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
    buildMenuActions();
}

void MainWindowController::buildMenuActions()
{
    QList<QAction *> menuActions;

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

void MainWindowController::onAbout()
{
    AboutDialog *aboutDialog = new AboutDialog(_window);
    aboutDialog->setAttribute(Qt::WA_DeleteOnClose);
    _window->showModalWidget(aboutDialog);
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
