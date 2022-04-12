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

} // namespace OCC::Wizard
