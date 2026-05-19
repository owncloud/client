#pragma once

#include <QMainWindow>

class QToolBar;
class QToolButton;
class QStackedWidget;

namespace OCC {

class ModalWrapperWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow();

    QSize minimumSizeHint() const override;

    void setVisible(bool visible) override;

    void setMoreMenuActions(const QList<QAction *> &actions);

    void showModalWidget(ModalWrapperWidget *w);

private slots:
    void endModalWidget();

private:
    void buildWindow();

    QToolBar *_toolbar = nullptr;
    QStackedWidget *_widgetStack = nullptr;

    QToolButton *_moreButton = nullptr;

    // hash of panel id's to panel widgets. the id is encapsulated in the main window's toolbar and menu actions
    // when the window "hears" that an action was triggered, it should just show the widget associated with the
    // action's id.
    // this will need refinement for the elements that currently run in the "modal" impls - maybe those go away
    // completely, no idea yet. but this is a good start so that the main window isn't controlling any of the activities related
    // to the panels. they need to control themselves!
    QHash<int, QWidget *> _panelLookup;
};
}
