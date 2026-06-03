#include "mainwindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>

#include "configfile.h"
#include "modalwrapperwidget.h"
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
    // call the override *for the class being constructed*. That is what we want so there is no problem here.
    // this is part of the reason we have tidy turned off - it's a bit too touchy.
    // question is, why is tidy butting in if we have it turned off? Figure this out later.
    setMinimumSize(minimumSizeHint());

    _actionGroup = new QActionGroup(this);
    _actionGroup->setExclusive(true);

    _accountsToolbar = new QToolBar(this);
    _accountsToolbar->setFocusPolicy(Qt::StrongFocus);
    _accountsToolbar->setObjectName("mainWindowAccountsToolbar");
    _accountsToolbar->setMovable(false);
    // 32 looks Baaaaaad
    _accountsToolbar->setIconSize(QSize(24, 24));
    _accountsToolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    addToolBar(_accountsToolbar);

    _toolbar = new QToolBar(this);
    _toolbar->setFocusPolicy(Qt::StrongFocus);
    _toolbar->setObjectName("mainWindowToolbar");
    _toolbar->setMovable(false);
    _toolbar->setAccessibleDescription(tr("Main toolbar for the application"));
    _toolbar->setAccessibleName(tr("Main toolbar"));
    addToolBar(_toolbar);

    QWidget *toolbarStretch = new QWidget(this);
    // who knows, maybe someday we use the toolbar vertically
    toolbarStretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _toolbar->addWidget(toolbarStretch);

    _separatorAction = new QAction(this);
    _separatorAction->setSeparator(true);

    _toolbar->addAction(_separatorAction);

    _moreButton = new QToolButton(this);
    // in theory yes, but the icon is hideously large - needs more space as border, maybe use the "no space" icon as example
    // proportions
    // _moreButton->setIcon(Resources::getCoreIcon("more"));
    _moreButton->setText("⋯");
    _moreButton->setPopupMode(QToolButton::InstantPopup);
    _toolbar->addWidget(_moreButton);

    _widgetStack = new QStackedWidget(this);
    setCentralWidget(_widgetStack);
}

void MainWindow::setMoreMenuActions(const QList<QAction *> &actions)
{
    _moreButton->addActions(actions);
}

void MainWindow::startModal()
{
    _accountsToolbar->setEnabled(false);
    _toolbar->setEnabled(false);
}

void MainWindow::stopModal()
{
    _accountsToolbar->setEnabled(true);
    _toolbar->setEnabled(true);
}

void MainWindow::showModalWidget(ModalWrapperWidget *w)
{
    // ownCloudGui::raise();
    // not sure if we should have an assert here but for real, the incoming modal widget should
    // never be in the current stack
    Q_ASSERT(_widgetStack->indexOf(w) == -1);

    connect(w, &ModalWrapperWidget::finished, this, &MainWindow::endModalWidget);
    _widgetStack->addWidget(w);
    _widgetStack->setCurrentWidget(w);
    startModal();
}

void MainWindow::endModalWidget()
{
    QWidget *target = qobject_cast<QWidget *>(sender());
    if (target == nullptr)
        return;

    if (_widgetStack->indexOf(target) >= 0)
        _widgetStack->removeWidget(target);
    target->deleteLater();
    stopModal();
}

void MainWindow::addPanelAction(QAction *action)
{
    QWidget *widget = action->data().value<QWidget *>();
    if (widget) {
        _toolbar->insertAction(_separatorAction, action);
        configurePanelAction(action);
    }
}

void MainWindow::addAccountAction(QAction *action)
{
    QWidget *widget = action->data().value<QWidget *>();
    if (widget) {
        _accountsToolbar->addAction(action);
        configurePanelAction(action);
    }
}

void MainWindow::configurePanelAction(QAction *action)
{
    QWidget *widget = action->data().value<QWidget *>();
    if (widget) {
        connect(action, &QAction::toggled, this, &MainWindow::onViewActionTriggered);
        _widgetStack->addWidget(widget);
        _widgetStack->setCurrentWidget(widget);
        _actionGroup->addAction(action);
    }
}
void MainWindow::removeAccountAction(QAction *action)
{
    QWidget *widget = action->data().value<QWidget *>();
    if (widget) {
        disconnect(action, nullptr, this, nullptr);
        _accountsToolbar->removeAction(action);
        _widgetStack->removeWidget(widget);
        // something should just happen. Let's see what it is.
        // _widgetStack->setCurrentWidget(widget);
    }
}

void MainWindow::onViewActionTriggered(bool selected)
{
    if (!selected)
        return;

    QAction *sourceAction = qobject_cast<QAction *>(sender());
    if (sourceAction) {
        QWidget *widget = sourceAction->data().value<QWidget *>();
        if (widget)
            _widgetStack->setCurrentWidget(widget);
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
}
