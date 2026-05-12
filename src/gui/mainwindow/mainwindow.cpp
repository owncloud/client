#include "mainwindow.h"

namespace OCC {
MainWindow::MainWindow()
    : QMainWindow{nullptr}
{
}

// envisioning:
// the actions for changing the current panel in the main window have a panel id as data
// so the main window controller just needs to find this id in the panel collection and show it.
// building the panel, its controller, and "installing" it on the main window with it's id should be up to
// an application (window) builder so the main window isn't responsible for knowing how many or which panels live in it.
// this is very powerful when it comes to adding or removing guis depending on....stuff.


}
