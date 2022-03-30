#include "accountconfiguredwizardpage.h"

#include "theme.h"
#include "ui_accountconfiguredwizardpage.h"

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

namespace OCC::Wizard {

AccountConfiguredWizardPage::AccountConfiguredWizardPage(const QString &defaultSyncTargetDir, bool vfsIsAvailable, bool enableVfsByDefault, bool vfsModeIsExperimental)
    : _ui(new ::Ui::AccountConfiguredWizardPage)
{
    _ui->setupUi(this);

    // by default, sync everything to an automatically chosen directory, VFS use depends on the OS
    // the defaults are provided by the controller
    _ui->localDirectoryLineEdit->setText(QDir::toNativeSeparators(defaultSyncTargetDir));
    _ui->syncEverythingRadioButton->setChecked(true);

    // could also make it invisible, but then the UX is different for different installations
    // this may be overwritten by a branding option (see below)
    _ui->useVfsRadioButton->setEnabled(vfsIsAvailable);

    _ui->useVfsRadioButton->setText(tr("Use &virtual files instead of downloading content immediately"));
    if (vfsModeIsExperimental) {
        _ui->useVfsRadioButton->setIcon(QIcon(QStringLiteral(":/client/resources/light/warning.svg")));
    }

    // just adjusting the visibility should be sufficient for these branding options
    if (Theme::instance()->wizardSkipAdvancedPage()) {
        _ui->advancedConfigSeparatorLine->setVisible(false);
        _ui->advancedConfigGroupBox->setVisible(false);
    }

    if (!Theme::instance()->showVirtualFilesOption()) {
        _ui->useVfsRadioButton->setVisible(false);
        vfsIsAvailable = false;
    }

    if (!vfsIsAvailable) {
        enableVfsByDefault = false;
    }

    if (enableVfsByDefault) {
        _ui->useVfsRadioButton->setChecked(true);

        // move up top
        _ui->syncModeGroupBoxLayout->removeWidget(_ui->useVfsRadioButton);
        _ui->syncModeGroupBoxLayout->insertWidget(0, _ui->useVfsRadioButton);
    }

    if (!vfsIsAvailable) {
        // fallback: it's set as default option in Qt Designer, but we should make sure the option is selected if VFS is not available
        _ui->syncEverythingRadioButton->setChecked(true);

        _ui->useVfsRadioButton->setToolTip(tr("The virtual filesystem feature is not available for this installation."));
    } else if (vfsModeIsExperimental) {
        _ui->useVfsRadioButton->setToolTip(tr("The virtual filesystem feature is not stable yet. Use with caution."));
    }

    connect(_ui->chooseLocalDirectoryButton, &QToolButton::clicked, this, [=]() {
        auto dialog = new QFileDialog(this, tr("Choose"), defaultSyncTargetDir);
        dialog->setFileMode(QFileDialog::Directory);
        dialog->setOption(QFileDialog::ShowDirsOnly);

        connect(dialog, &QFileDialog::fileSelected, this, [this](const QString &directory) {
            // the directory chooser should guarantee that the directory exists
            Q_ASSERT(QDir(directory).exists());

            _ui->localDirectoryLineEdit->setText(QDir::toNativeSeparators(directory));
        });

        dialog->show();
    });

    // this should be handled on application startup, too
    OC_ENFORCE(!Theme::instance()->forceVirtualFilesOption() || vfsIsAvailable);

    if (Theme::instance()->forceVirtualFilesOption()) {
        // this has no visual effect, but is needed for syncMode()
        _ui->useVfsRadioButton->setChecked(true);

        // we want to hide the entire sync mode selection from the user, not just disable it
        _ui->syncModeGroupBox->setVisible(false);
    }

    connect(_ui->advancedConfigGroupBox, &QGroupBox::toggled, this, [this](bool enabled) {
        // layouts cannot be hidden, therefore we use a plain widget within the group box to "house" the contained widgets
        _ui->advancedConfigGroupBoxContentWidget->setVisible(enabled);

        // could not find a better way to hide the frame on demand, which is needed to hide the widget completely
        _ui->advancedConfigGroupBox->setStyleSheet(enabled ? QString() : QStringLiteral("QGroupBox#advancedConfigGroupBox{border: 0}"));
    });

    if (vfsModeIsExperimental) {
        connect(_ui->useVfsRadioButton, &QRadioButton::clicked, this, [this]() {
            auto messageBox = new QMessageBox(
                QMessageBox::Warning,
                tr("Enable experimental feature?"),
                tr("When the \"virtual files\" mode is enabled no files will be downloaded initially. "
                   "Instead, a tiny file will be created for each file that exists on the server. "
                   "The contents can be downloaded by running these files or by using their context menu."
                   "\n\n"
                   "The virtual files mode is mutually exclusive with selective sync. "
                   "Currently unselected folders will be translated to online-only folders "
                   "and your selective sync settings will be reset."
                   "\n\n"
                   "Switching to this mode will abort any currently running synchronization."
                   "\n\n"
                   "This is a new, experimental mode. If you decide to use it, please report any "
                   "issues that come up."),
                QMessageBox::NoButton,
                this);

            messageBox->addButton(tr("Enable experimental placeholder mode"), QMessageBox::AcceptRole);
            messageBox->addButton(tr("Stay safe"), QMessageBox::RejectRole);

            messageBox->setAttribute(Qt::WA_DeleteOnClose);

            connect(messageBox, &QMessageBox::rejected, this, [this]() {
                // bring back to "safety"
                _ui->syncEverythingRadioButton->setChecked(true);
            });

            messageBox->show();
        });
    }

    // toggle once to have the according handlers set up the initial UI state
    _ui->advancedConfigGroupBox->setChecked(true);
    _ui->advancedConfigGroupBox->setChecked(false);
}

AccountConfiguredWizardPage::~AccountConfiguredWizardPage() noexcept
{
    delete _ui;
}

QString AccountConfiguredWizardPage::syncTargetDir() const
{
    return QDir::toNativeSeparators(_ui->localDirectoryLineEdit->text());
}

SyncMode AccountConfiguredWizardPage::syncMode() const
{
    if (_ui->syncEverythingRadioButton->isChecked()) {
        return SyncMode::SyncEverything;
    }
    if (_ui->selectiveSyncRadioButton->isChecked()) {
        return SyncMode::SelectiveSync;
    }
    if (_ui->useVfsRadioButton->isChecked()) {
        return SyncMode::UseVfs;
    }

    Q_UNREACHABLE();
}
}
