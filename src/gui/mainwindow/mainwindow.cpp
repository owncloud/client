#include "mainwindow.h"

#include <QApplication>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>

#include "configfile.h"
#include "theme.h"

#ifdef Q_OS_MAC
#include "settingsdialog_mac.h"
void setActivationPolicy(ActivationPolicy policy);
#endif

namespace OCC {
MainWindow::MainWindow()
    : QMainWindow{nullptr}
{
    // required as group for saveGeometry call. the original name is "Settings" so we should factor this in
    // and make sure we either accept that previous geometry is lost between upgrade/downgrades OR put the name back to Settings
    // which imo is a terrible name but we could save the rename til next major version.
    // in reality the stored geometry is just the size afaik but will double check. If we had user editable docking positions,
    // subwidgets that can run along side the main window, etc. the stored geometry would be more important for user happiness.
    setObjectName(QStringLiteral("MainWindow"));
    setWindowTitle(Theme::instance()->appNameGUI());

    // People perceive this as a Window, so also make Ctrl+W work
    addAction(tr("Hide"), Qt::CTRL | Qt::Key_W, this, &MainWindow::hide);

    ConfigFile().restoreGeometry(this);
#ifdef Q_OS_MAC
    setActivationPolicy(ActivationPolicy::Accessory);
#endif

    buildWindow();
}

void MainWindow::buildWindow()
{
    // clang is complaining that call to minimumSizeHint bypasses virtual dispatch, but I'm not seeing any problem.
    // the link to docs for that warning = 404 :D
    // regardless, the evaluation is misleading as according to c++ standard, calling a virtual function in a ctr is guaranteed to
    // call the override for the class being constructed. There is no problem here.
    // this is part of the reason we have tidy turned off - it's a bit too touchy.
    // question is, why is tidy butting in if we have it turned off? Figure this out later.
    setMinimumSize(minimumSizeHint());

    _toolbar = new QToolBar(this);
    _toolbar->setObjectName("mainWindowToolbar");
    _toolbar->setMovable(false);
    _toolbar->setAccessibleDescription(tr("Main toolbar for the application"));
    _toolbar->setAccessibleName(tr("Main toolbar"));
    addToolBar(_toolbar);

    QWidget *toolbarStretch = new QWidget(this);
    // who knows, maybe someday we use the toolbar vertically
    toolbarStretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _toolbar->addWidget(toolbarStretch);

    _toolbar->addSeparator();

    _moreButton = new QToolButton(this);
    _moreButton->setText("⋯");
    //  _moreButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    _moreButton->setPopupMode(QToolButton::InstantPopup);
    _toolbar->addWidget(_moreButton);

    _widgetStack = new QStackedWidget(this);
    setCentralWidget(_widgetStack);
}

void MainWindow::setMoreMenuActions(const QList<QAction *> &actions)
{
    _moreButton->addActions(actions);
}

void MainWindow::showModalWidget(QWidget *w)
{
    // ownCloudGui::raise();
    if (_widgetStack->indexOf(w) == -1) {
        _widgetStack->addWidget(w);
        _widgetStack->setCurrentWidget(w);
    }
}

QSize MainWindow::minimumSizeHint() const
{
    const QSize min{800, 700}; // When changing this, please check macOS: widgets there have larger insets, so they take up more space.
    const auto screen = windowHandle() ? windowHandle()->screen() : QApplication::screenAt(QCursor::pos());
    if (screen) {
        const auto availableSize = screen->availableSize();
        if (availableSize.isValid()) {
            // Assume we can use at least 90% of the screen, if the screen is smaller than 800x700 pixels.
            //
            // Note: this means that the wizards have even less space: with the style we use, the
            // wizard tries to fit inside the window. So, if this is a common case that users have
            // such small screens, and the contents of the wizard screen are squashed together (or
            // not shown due to lack of space), we should consider putting that content in a
            // scroll-view.
            return min.boundedTo(availableSize * 0.9);
        }
    }
    return min;
}

void MainWindow::setVisible(bool visible)
{
    if (!visible) {
        ConfigFile cfg;
        cfg.saveGeometry(this);
    }

#ifdef Q_OS_MAC
    if (visible) {
        setActivationPolicy(ActivationPolicy::Regular);
    } else {
        setActivationPolicy(ActivationPolicy::Accessory);
    }
#endif

    QMainWindow::setVisible(visible);
}
// envisioning:
// the actions for changing the current panel in the main window have a panel id as data
// so the main window controller just needs to find this id in the panel collection and show it.
// building the panel, its controller, and "installing" it on the main window with it's id should be up to
// an application (window) builder so the main window isn't responsible for knowing how many or which panels live in it.
// this is very powerful when it comes to adding or removing guis depending on....stuff.


}
