#include "setupwizardcontroller.h"

#include "gui/accountmanager.h"
#include "gui/networkadapters/fetchcapabilitiesadapter.h"
#include "pages/accountconfiguredwizardpage.h"
#include "states/abstractsetupwizardstate.h"
#include "states/accountconfiguredsetupwizardstate.h"
#include "states/oauthcredentialssetupwizardstate.h"
#include "states/serverurlsetupwizardstate.h"

using namespace std::chrono_literals;

namespace {

using namespace OCC;
using namespace OCC::Wizard;

using namespace SetupWizardControllerPrivate;

/**
 * Generate list of wizard states to put in the navigation.
 * The actual wizard may be in states not within this list to perform tasks in the background without user interaction
 * (e.g., to detect the authentication method the server uses).
 */
QList<SetupWizardState> getNavigationEntries()
{
    QList<SetupWizardState> states = {
        SetupWizardState::ServerUrlState,
        SetupWizardState::CredentialsState,
        SetupWizardState::AccountConfiguredState,
    };

    return states;
}

}

namespace OCC::Wizard {

Q_LOGGING_CATEGORY(lcSetupWizardController, "gui.setupwizard.controller")

SetupWizardController::SetupWizardController(SettingsDialog *parent)
    : QObject(parent)
    , _context(new SetupWizardContext(parent, this))
{
    _context->window()->setNavigationEntries(getNavigationEntries());

    // we always switch to this state, even if the URL is overridden by the theme
    // it will detect
    changeStateTo(SetupWizardState::FirstState);

    // allow settings dialog to clean up the wizard controller and all the objects it created
    connect(_context->window(), &SetupWizardWidget::rejected, this, [this]() {
        qCDebug(lcSetupWizardController) << "wizard window closed";
        Q_EMIT finished(nullptr, SyncMode::Invalid, {});
    });

    connect(_context->window(), &SetupWizardWidget::navigationEntryClicked, this, [this](SetupWizardState clickedState) {
        qCDebug(lcSetupWizardController) << "pagination entry clicked: current state" << _currentState << "clicked state" << clickedState;
        changeStateTo(clickedState);
    });

    connect(_context->window(), &SetupWizardWidget::nextButtonClicked, this, [this]() {
        qCDebug(lcSetupWizardController) << "next button clicked, current state" << _currentState;
        _currentState->evaluatePage();
    });

    // in case the back button is clicked, the current page's data is dismissed, and the previous page should be shown
    connect(_context->window(), &SetupWizardWidget::backButtonClicked, this, [this]() {
        // with enum classes, we have to explicitly cast to a numeric value
        const auto currentStateIdx = static_cast<int>(_currentState->state());
        Q_ASSERT(currentStateIdx > 0);

        qCDebug(lcSetupWizardController) << "back button clicked, current state" << _currentState;

        auto previousState = static_cast<SetupWizardState>(currentStateIdx - 1);

        changeStateTo(previousState);
    });
}

SetupWizardWidget *SetupWizardController::window()
{
    return _context->window();
}

void SetupWizardController::changeStateTo(SetupWizardState nextState, ChangeReason reason)
{
    // validate initial state
    Q_ASSERT(nextState == SetupWizardState::ServerUrlState || _currentState != nullptr);

    if (_currentState != nullptr) {
        _currentState->deleteLater();
    }

    switch (nextState) {
    case SetupWizardState::ServerUrlState: {
        _currentState = new ServerUrlSetupWizardState(_context);
        break;
    }
    case SetupWizardState::CredentialsState: {
            _currentState = new OAuthCredentialsSetupWizardState(_context);
            break;
    }
    case SetupWizardState::AccountConfiguredState: {
        _currentState = new AccountConfiguredSetupWizardState(_context);

        switch (reason) {
        case ChangeReason::Default:
            break;
        case ChangeReason::EvaluationFailed:
            // whenever the evaluation of the last page fails, it's safe to assume it's due to some issue with the advanced
            // therefore, we want to show them in that case
            auto *page = dynamic_cast<AccountConfiguredWizardPage *>(_currentState->page());
            if (OC_ENSURE(page != nullptr)) {
                page->setShowAdvancedSettings(true);
            }
        }
        break;
    }
    default:
        Q_UNREACHABLE();
    }

    // todo: something REALLY weird going on here. if I don't in
    if (_currentState == nullptr)
        return;

    Q_ASSERT(_currentState != nullptr);
    Q_ASSERT(_currentState->state() == nextState);

    qCDebug(lcSetupWizardController) << "Current wizard state:" << _currentState->state();

    connect(_currentState, &AbstractSetupWizardState::evaluationSuccessful, this, [this]() {
        switch (_currentState->state()) {
        case SetupWizardState::ServerUrlState: {
                changeStateTo(SetupWizardState::CredentialsState);
            return;
        }
        case SetupWizardState::CredentialsState: {
            // for now, we assume there is only a single instance
            const auto webFingerInstances = _context->accountBuilder().webFingerInstances();
            if (!webFingerInstances.isEmpty()) {
                Q_ASSERT(webFingerInstances.size() == 1);
                _context->accountBuilder().setWebFingerSelectedInstance(webFingerInstances.front());
            }

            // not a fan of performing this job here, should be moved into its own (headless) state IMO
            // we can bind it to the current state, which will be cleaned up by changeStateTo(...) as soon as the job finished
            auto fetchUserInfoJob = _context->startFetchUserInfoJob(_currentState);

            connect(fetchUserInfoJob, &CoreJob::finished, this, [this, fetchUserInfoJob] {
                if (fetchUserInfoJob->success()) {
                    auto result = fetchUserInfoJob->result().value<FetchUserInfoResult>();
                    if (AccountManager::instance()->accountForLoginExists(_context->accountBuilder().serverUrl(), result.userName())) {
                        _context->window()->showErrorMessage(tr("You are already connected to an account with these credentials."));
                        changeStateTo(_currentState->state());
                    } else {
                        _context->accountBuilder().setDisplayName(result.displayName());
                        _context->accountBuilder().authenticationStrategy()->setDavUser(result.userName());

                        const QString token = _context->accountBuilder().authenticationStrategy()->token();
                        Q_ASSERT(!token.isEmpty());
                        // get the capabilities so we can block oc10 accounts. Checking for spaces support is not 
                        // great and this should be refined, but for now it's effective.
                        FetchCapabilitiesAdapter fetchCapabilities(_context->accessManager(), token, _context->userInfoUrl());
                        FetchCapabilitiesResult capabilitiesResult = fetchCapabilities.getResult();
                        if (!capabilitiesResult.success()) {
                            // I don't think we want to display the core error message as it's stuff like json errors and not
                            // useful to the user but we can change this after we have the discussion about error messages
                            _context->window()->showErrorMessage(tr("Unable to retrieve capabilities from server"));
                            changeStateTo(_currentState->state());
                        }
                        if (!capabilitiesResult.capabilities.spacesSupport().enabled) {
                            _context->window()->showErrorMessage(tr("The server is not supported by this client"));
                            changeStateTo(_currentState->state());
                        } else
                            changeStateTo(SetupWizardState::AccountConfiguredState);
                        // todo: #27: new wizard should take the capabilities and add them to the account builder or equivalent!
                    }
                } else if (fetchUserInfoJob->reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401) {
                    _context->window()->showErrorMessage(tr("Invalid credentials."));
                    changeStateTo(_currentState->state());
                } else {
                    _context->window()->showErrorMessage(tr("Failed to retrieve user information from server."));
                    changeStateTo(_currentState->state());
                }
            });

            return;
        }
        case SetupWizardState::AccountConfiguredState: {
            // Refactoring todo: is that page guaranteed to be alive at this point? more: why on earth are we pulling data from the page?!
            // that info should be transferred to the state or some other data model or dm owner
            // using a polymorphic impl for the states/pages is in theory good, but if you have to start casting to specific
            // impls that is a big code smell that indicates your abstraction is faulty
            const auto *pagePtr = qobject_cast<AccountConfiguredWizardPage *>(_currentState->page());
            Q_ASSERT(pagePtr);

            auto account = _context->accountBuilder().build();
            Q_ASSERT(account != nullptr);
            if (pagePtr && account) {
                Q_EMIT finished(account, pagePtr->syncMode(), _context->accountBuilder().dynamicRegistrationData());
            }
            return;
        }
        default:
            Q_UNREACHABLE();
        }
    });

    connect(_currentState, &AbstractSetupWizardState::evaluationFailed, this, [this](const QString &errorMessage) {
        _context->window()->showErrorMessage(errorMessage);
        changeStateTo(_currentState->state(), ChangeReason::EvaluationFailed);
    });

    _context->window()->displayPage(_currentState->page(), _currentState->state());
}

SetupWizardController::~SetupWizardController() noexcept { }
}
