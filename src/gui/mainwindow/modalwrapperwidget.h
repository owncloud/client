#pragma once

#include <QWidget>

namespace OCC {

class ModalWrapperWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ModalWrapperWidget(QWidget *content, QWidget *parent);

signals:
    void finished();
};

}
