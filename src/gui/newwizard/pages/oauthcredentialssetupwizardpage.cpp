#include "oauthcredentialssetupwizardpage.h"

#include "theme.h"
#include "ui_oauthcredentialssetupwizardpage.h"

namespace OCC::Wizard {

OAuthCredentialsSetupWizardPage::OAuthCredentialsSetupWizardPage(const QUrl &serverUrl)
    : _ui(new ::Ui::OAuthCredentialsSetupWizardPage)
{
    _ui->setupUi(this);

    _ui->urlLabel->setText(tr("Connecting to <a href='%1'>%1</a>").arg(serverUrl.toString()));

    // we want to give the user a chance to preserve their privacy when using a private proxy for instance
    // therefore, we need to make sure the user can manually
    // using clicked allows a user to "abort the click" (unlike pressed and released)
    connect(_ui->openBrowserButton, &QPushButton::clicked, this, [this]() {
        Q_EMIT openBrowserButtonPushed();

        // change button title after first click
        _ui->openBrowserButton->setText(tr("Reopen Browser"));
    });
    connect(_ui->copyUrlToClipboardButton, &QToolButton::clicked, this, [this]() {
        Q_EMIT copyUrlToClipboardButtonPushed();
    });

    connect(this, &AbstractSetupWizardPage::pageDisplayed, this, [this]() {
        _ui->openBrowserButton->setFocus();
    });

    _ui->pleaseLogIntoLabel->setText(tr("Please use your browser to log into %1").arg(Theme::instance()->appNameGUI()));
}

void OAuthCredentialsSetupWizardPage::disableButtons()
{
    _ui->openBrowserButton->setEnabled(false);
    _ui->copyUrlToClipboardButton->setEnabled(false);
}

OAuthCredentialsSetupWizardPage::~OAuthCredentialsSetupWizardPage()
{
    delete _ui;
}

bool OAuthCredentialsSetupWizardPage::inputValidated()
{
    // in this special case, the input may never be validated, i.e., the next button also never needs to be enabled
    // an external system set up by the controller will move to the next page in the background
    return false;
}

} // namespace OCC::Wizard
