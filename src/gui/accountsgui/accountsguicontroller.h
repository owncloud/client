#pragma once

#include <QObject>

namespace OCC {
class AccountsGuiController : public QObject
{
    Q_OBJECT

public:
    AccountsGuiController(QObject *parent);
};
}
