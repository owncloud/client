#include "owncloudusernamepage.h"
#include "ui_owncloudusernamepage.h"

#include "creds/webfinger.h"
#include "theme.h"
#include "wizard/owncloudwizard.h"

#include "account.h"

using namespace OCC;

OwncloudUserNamePage::OwncloudUserNamePage(OwncloudWizard *parent)
    : AbstractWizardPage(parent)
    , ui(new Ui::OwncloudUserNamePage)
{
    ui->setupUi(this);

    Theme *theme = Theme::instance();
    setTitle(WizardCommon::titleTemplate().arg(tr("Connect to %1").arg(theme->appNameGUI())));
    setSubTitle(WizardCommon::subTitleTemplate().arg(tr("Enter %1").arg(theme->enumToDisplayName(theme->userIDType()))));
    ui->usernameLabel->setText(theme->enumToDisplayName(theme->userIDType()));
    ui->leUsername->setPlaceholderText(theme->userIDHint());
}

OwncloudUserNamePage::~OwncloudUserNamePage()
{
    delete ui;
}

bool OwncloudUserNamePage::isComplete() const
{
    return !ui->leUsername->text().isEmpty();
}

bool OwncloudUserNamePage::validatePage()
{
    if (_validated) {
        return true;
    }
    // prevent multiple ruinning jobs
    if (_webFingerJob) {
        return false;
    }

    const QString user = ui->leUsername->text().simplified();
    owncloudWizard()->setUser(user);
    if (user.isEmpty()) {
        return false;
    }
    owncloudWizard()->account()->setDavUser(user);
    ui->progressIndicator->startAnimation();

    const QUrl url = owncloudWizard()->ocUrl();
    _webFingerJob = new WebFinger(owncloudWizard()->account()->networkAccessManager(), this);
    connect(_webFingerJob, &WebFinger::finished, this, [this] {
        auto url = owncloudWizard()->account()->url();
        if (_webFingerJob->error().error == QJsonParseError::NoError) {
            url = _webFingerJob->href();
        }
        stopWebFinger();
        // will trigger completeChanged
        Q_EMIT owncloudWizard()->determineAuthType(url.toString());
    });
    _webFingerJob->start(url, user);
    return false;
}

void OwncloudUserNamePage::initializePage()
{
    connect(ui->leUsername, &QLineEdit::textChanged, this, [this] {
        _validated = false;
        stopWebFinger();
        Q_EMIT completeChanged();
    });

    connect(owncloudWizard(), &OwncloudWizard::authTypeChanged, this, [this] {
        ui->progressIndicator->stopAnimation();
        _validated = owncloudWizard()->authType() != DetermineAuthTypeJob::AuthType::Unknown;
        if (_validated) {
            // hack to make the async nature of this dialog possible
            QTimer::singleShot(0, owncloudWizard(), &OwncloudWizard::next);
        }
    });

    connect(owncloudWizard(), &OwncloudWizard::userChanged, this, [this] {
        _validated = false;
        Q_EMIT completeChanged();
    });
}


void OwncloudUserNamePage::stopWebFinger()
{
    _webFingerJob->deleteLater();
    _webFingerJob = nullptr;
}