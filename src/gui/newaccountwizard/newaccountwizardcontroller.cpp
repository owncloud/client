#include "newaccountwizardcontroller.h"

#include <QWizard>

#include "newaccountmodel.h"


NewAccountWizardController::NewAccountWizardController(NewAccountModel *model, QWizard *view, QObject *parent)
    : QObject{parent}
    , _model(model)
    , _wizard(view)
{
}
