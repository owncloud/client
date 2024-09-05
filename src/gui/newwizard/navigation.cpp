#include "navigation.h"

#include <QDebug>
#include <QRadioButton>

namespace OCC::Wizard {

Navigation::Navigation(QWidget *parent)
    : OCQuickWidget(parent)
{
    setOCContext(QUrl(QStringLiteral("qrc:/qt/qml/org/ownCloud/gui/newwizard/qml/Navigation.qml")), this->parentWidget(), this, QJSEngine::CppOwnership);
}

void Navigation::setStates(const QList<SetupWizardState> &newEntries)
{
    if (_states != newEntries) {
        _states = newEntries;
        Q_EMIT statesChanged();
    }
}

QList<SetupWizardState> Navigation::states() const
{
    return _states;
}

void Navigation::setActiveState(SetupWizardState newState)
{
    if (_activeState != newState) {
        _activeState = newState;
        Q_EMIT activeStatesChanged();
    }
}

SetupWizardState Navigation::activeState() const
{
    return _activeState;
}

QString Navigation::stateDisplayName(SetupWizardState state) const
{
    return Utility::enumToDisplayName(state);
}

}
