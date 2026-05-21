#pragma once

#include <QObject>

namespace OCC {

class AccountView;

class AccountViewController : public QObject
{
    Q_OBJECT
public:
    explicit AccountViewController(AccountView *view, QObject *parent = nullptr);

signals:

private:
    AccountView *_view = nullptr;
};

}
