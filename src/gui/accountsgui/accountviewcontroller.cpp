#include "accountviewcontroller.h"

#include "accountview.h"

namespace OCC {

AccountViewController::AccountViewController(AccountView *view, QObject *parent)
    : QObject{parent}
    , _view(view)
{
}

}
