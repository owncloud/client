#pragma once

#include <QObject>

namespace OCC {

class MainWindow;

class MainWindowController : public QObject
{
    Q_OBJECT
public:
    explicit MainWindowController(MainWindow *window, QObject *parent = nullptr);

    // public for now
    void setup();

private:
    void buildMenuActions();

    void onAbout();
    void onQuit();

    MainWindow *_window = nullptr;
};

}
