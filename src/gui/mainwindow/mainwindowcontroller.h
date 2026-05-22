#pragma once

#include <QObject>

namespace OCC {

class MainWindow;
class AccountsGuiController;

class MainWindowController : public QObject
{
    Q_OBJECT
public:
    explicit MainWindowController(MainWindow *window, QObject *parent = nullptr);

    // public for now
    void setup();

private:
    void buildMenuActions();

    void onAddAccount();
    void onSettings();
    void onAbout();
    void onQuit();

    MainWindow *_window = nullptr;
    AccountsGuiController *_accountsController = nullptr;
};

}
