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

    void addAccountAction(QAction *action);
    void removeAccountAction(QAction *action);

private slots:
    void endModalWidget();
    void onViewActionTriggered(bool selected);

private:
    void buildWindow();

    QToolBar *_accountsToolbar = nullptr;
    QToolBar *_toolbar = nullptr;
    QStackedWidget *_widgetStack = nullptr;

    QToolButton *_moreButton = nullptr;
};
}
