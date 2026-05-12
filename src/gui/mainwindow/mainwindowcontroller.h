#pragma once

#include <QObject>

namespace OCC {

class MainWindowController : public QObject
{
    Q_OBJECT
public:
    explicit MainWindowController(QObject *parent = nullptr);

signals:
};

}
