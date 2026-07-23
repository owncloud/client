/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "folderwizardlocalpath.h"
#include "ui_folderwizardsourcepage.h"

#include "folderwizard.h"
#include "folderwizard_p.h"

#include "gui/folderman.h"

#include <QDir>
#include <QFileDialog>
#include <QStandardPaths>

using namespace OCC;

FolderWizardLocalPath::FolderWizardLocalPath(FolderWizardPrivate *parent)
    : FolderWizardPage(parent)
    , _ui(new Ui_FolderWizardSourcePage)
{
    _ui->setupUi(this);
    registerField(QStringLiteral("sourceFolder*"), _ui->localFolderLineEdit);
    connect(_ui->localFolderChooseBtn, &QAbstractButton::clicked, this, &FolderWizardLocalPath::slotChooseLocalFolder);
    _ui->localFolderChooseBtn->setToolTip(tr("Click to select a local folder to sync."));

    _ui->localFolderLineEdit->setToolTip(tr("Enter the path to the local folder."));

    _ui->warnLabel->setTextFormat(Qt::RichText);
    _ui->warnLabel->hide();
}

FolderWizardLocalPath::~FolderWizardLocalPath()
{
    delete _ui;
}

void FolderWizardLocalPath::initializePage()
{
    _ui->warnLabel->hide();
    _ui->localFolderLineEdit->setText(QDir::toNativeSeparators(folderWizardPrivate()->initialLocalPath()));
}

QString FolderWizardLocalPath::localPath() const
{
    return QDir::fromNativeSeparators(_ui->localFolderLineEdit->text());
}

bool FolderWizardLocalPath::isComplete() const
{
    // todo: DC-219 we need a check here to see if the chosen local folder path supports vfs, provided vfs is generally available,
    // and reject the path with reason it failed if it's no good. Regardless of whether the user currently wants to use vfs or not,
    // we should not accept local paths that *can't* support vfs if they change their mind later.
    // use Vfs::pathSupportDetail to get the reason vfs is not supported (path is supported if the return string is empty)
    auto folderType = FolderMan::NewFolderType::SpacesFolder;
    auto accountUuid = folderWizardPrivate()->uuid();
    QString errorStr = FolderMan::instance()->checkPathValidity(localPath(), folderType, accountUuid);

    bool isOk = errorStr.isEmpty();
    QStringList warnStrings;
    if (!isOk) {
        warnStrings << errorStr;
    }

    _ui->warnLabel->setWordWrap(true);
    if (isOk) {
        _ui->warnLabel->hide();
        _ui->warnLabel->clear();
    } else {
        _ui->warnLabel->show();
        QString warnings = FolderWizardPrivate::formatWarnings(warnStrings);
        _ui->warnLabel->setText(warnings);
    }
    return isOk;
}

void FolderWizardLocalPath::slotChooseLocalFolder()
{
    QString sf = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QDir d(sf);

    // open the first entry of the home dir. Otherwise the dir picker comes
    // up with the closed home dir icon, stupid Qt default...
    QStringList dirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks,
        QDir::DirsFirst | QDir::Name);

    if (dirs.count() > 0)
        sf += QLatin1Char('/') + dirs.at(0); // Take the first dir in home dir.

    QString dir = QFileDialog::getExistingDirectory(this,
        tr("Select the local folder"),
        sf);
    if (!dir.isEmpty()) {
        // set the last directory component name as alias
        _ui->localFolderLineEdit->setText(QDir::toNativeSeparators(dir));
    }
    Q_EMIT completeChanged();
}
