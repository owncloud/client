#include "owncloudusernamepage.h"
#include "ui_owncloudusernamepage.h"

#include "creds/webfinger.h"
#include "theme.h"
#include "wizard/owncloudwizard.h"

#include "account.h"

using namespace OCC;

OwncloudUserNamePage::OwncloudUserNamePage(QWidget *parent)
    : AbstractWizardPage(parent)
    , ui(new Ui::OwncloudUserNamePage)
{
    ui->setupUi(this);

    Theme *theme = Theme::instance();
    setTitle(WizardCommon::titleTemplate().arg(tr("Connect to %1").arg(theme->appNameGUI())));
    setSubTitle(WizardCommon::subTitleTemplate().arg(tr("Enter %1").arg(theme->enumToDisplayName(theme->userIDType()))));
    ui->usernameLabel->setText(theme->enumToDisplayName(theme->userIDType()));
    ui->leUsername->setPlaceholderText(theme->userIDHint());

    connect(ui->leUsername, &QLineEdit::editingFinished, this, [this] {
        owncloudWizard()->setUser(ui->leUsername->text());
        validatePage();
    });

    connect(owncloudWizard(), &OwncloudWizard::authTypeChanged, this, [this] {
        ui->progressIndicator->stopAnimation();
        _validated = owncloudWizard()->authType() != DetermineAuthTypeJob::AuthType::Unknown;
        Q_EMIT completeChanged();
    });
}

OwncloudUserNamePage::~OwncloudUserNamePage()
{
    delete ui;
}

void OwncloudUserNamePage::cleanupPage()
{
    owncloudWizard()->setUser(QString());
    ui->leUsername->clear();
}

bool OwncloudUserNamePage::isComplete() const
{
    return _validated && !ui->leUsername->text().isEmpty();
}

int OwncloudUserNamePage::nextId() const
{
    switch (owncloudWizard()->authType()) {
    case DetermineAuthTypeJob::AuthType::Basic:
        return WizardCommon::Page_HttpCreds;
    case DetermineAuthTypeJob::AuthType::OAuth:
        return WizardCommon::Page_OAuthCreds;
    case DetermineAuthTypeJob::AuthType::Unknown:
        return AbstractWizardPage::nextId();
    }
    Q_UNREACHABLE();
}

bool OwncloudUserNamePage::validatePage()
{
    if (isComplete()) {
        return true;
    }

    owncloudWizard()->account()->setDavUser(owncloudWizard()->user());
    if (true) {
        const QUrl url = owncloudWizard()->account()->url();
        Q_EMIT owncloudWizard()->determineAuthType(url.toString());
    } else {
        auto *webFinger = new WebFinger(owncloudWizard()->account(), this);
        connect(webFinger, &WebFinger::finished, this, [webFinger, this] {
            if (webFinger->error().error == QJsonParseError::NoError) {
                Q_EMIT owncloudWizard()->determineAuthType(webFinger->href().toString());
            }
        });
        webFinger->start(owncloudWizard()->account()->davUser());
    }
    return false;
}

void OwncloudUserNamePage::initializePage()
{
    // we'll require to rerun webfinger
    ui->progressIndicator->stopAnimation();
    _validated = false;
}
