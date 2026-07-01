/*
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "settingsview.h"
#include "ui_settingsview.h"

#include "application.h"
#include "ignorelisteditor.h"
#include "libsync/configfile.h"
#include "libsync/theme.h"
#include "logbrowser.h"
#include "translations.h"

#include <QMessageBox>
#include <QOperatingSystemVersion>
#include <QScopedValueRollback>

namespace OCC {

SettingsView::SettingsView(QWidget *parent)
    : QWidget(parent)
    , _ui(new Ui::SettingsView)
    , _currentlyLoading(false)
{
    _ui->setupUi(this);

    connect(_ui->desktopNotificationsCheckBox, &QAbstractButton::toggled, this, &SettingsView::slotToggleOptionalDesktopNotifications);

    reloadConfig();
    loadMiscSettings();

    // misc
    connect(_ui->monoIconsCheckBox, &QAbstractButton::toggled, this, &SettingsView::saveMiscSettings);
    connect(_ui->crashreporterCheckBox, &QAbstractButton::toggled, this, &SettingsView::saveMiscSettings);

    connect(_ui->languageDropdown, QOverload<int>::of(&QComboBox::activated), this, [this]() {
        // first, store selected language in config file
        saveMiscSettings();

        // warn user that a language change requires a restart to take effect
        QMessageBox::warning(this, tr("Warning"), tr("Language changes require a restart of this application to take effect."), QMessageBox::Ok);
    });

    /* handle the hidden file checkbox */

    /* the ignoreHiddenFiles flag is a folder specific setting, but for now, it is
     * handled globally. Save it to every folder that is defined.
     */
    connect(_ui->syncHiddenFilesCheckBox, &QCheckBox::toggled, this, [](bool checked) { FolderMan::instance()->setIgnoreHiddenFiles(!checked); });

    _ui->crashreporterCheckBox->setVisible(Theme::instance()->withCrashReporter());

    _ui->moveToTrashCheckBox->setVisible(true);
    connect(_ui->moveToTrashCheckBox, &QCheckBox::toggled, this, &SettingsView::moveToTrashChanged); /*[this](bool checked) {
         ConfigFile().setMoveToTrash(checked);
         Q_EMIT moveToTrashChanged(checked);
     });*/

    // OEM themes are not obliged to ship mono icons, so there
    // is no point in offering an option
    _ui->monoIconsCheckBox->setVisible(Resources::hasMonoTheme());

    connect(_ui->ignoredFilesButton, &QAbstractButton::clicked, this, &SettingsView::slotIgnoreFilesEditor);
    connect(_ui->logSettingsButton, &QPushButton::clicked, this, &SettingsView::slotShowLogSettings);

    if (!Theme::instance()->aboutShowCopyright()) {
        _ui->copyrightLabel->hide();
    }
}

SettingsView::~SettingsView()
{
    delete _ui;
}

void SettingsView::loadMiscSettings()
{
    QScopedValueRollback<bool> scope(_currentlyLoading, true);
    ConfigFile cfgFile;
    _ui->monoIconsCheckBox->setChecked(cfgFile.monoIcons());
    _ui->desktopNotificationsCheckBox->setChecked(cfgFile.optionalDesktopNotifications());
    _ui->crashreporterCheckBox->setChecked(cfgFile.crashReporter());
    _ui->monoIconsCheckBox->setChecked(cfgFile.monoIcons());

    // the dropdown has to be populated before we can can pick an entry below based on the stored setting
    loadLanguageNamesIntoDropdown();

    const auto &locale = cfgFile.uiLanguage();
    const auto index = _ui->languageDropdown->findData(locale);
    _ui->languageDropdown->setCurrentIndex(index < 0 ? 0 : index);
}

void SettingsView::showEvent(QShowEvent *)
{
    reloadConfig();
}

void SettingsView::saveMiscSettings()
{
    if (_currentlyLoading)
        return;
    ConfigFile cfgFile;
    bool isChecked = _ui->monoIconsCheckBox->isChecked();
    cfgFile.setMonoIcons(isChecked);
    Theme::instance()->setSystrayUseMonoIcons(isChecked);
    cfgFile.setCrashReporter(_ui->crashreporterCheckBox->isChecked());

    // the first entry, identified by index 0, means "use default", which is a special case handled below
    const QString pickedLocale = _ui->languageDropdown->currentData().toString();
    cfgFile.setUiLanguage(pickedLocale);
}

void SettingsView::slotToggleLaunchOnStartup(bool enable)
{
    Theme *theme = Theme::instance();
    Utility::setLaunchOnStartup(theme->appName(), theme->appNameGUI(), enable);
}

void SettingsView::slotToggleOptionalDesktopNotifications(bool enable)
{
    ConfigFile cfgFile;
    cfgFile.setOptionalDesktopNotifications(enable);
}

void SettingsView::slotIgnoreFilesEditor()
{
    if (_ignoreEditor.isNull()) {
        _ignoreEditor = new IgnoreListEditor(ocApp()->mainWindow());
        _ignoreEditor->setAttribute(Qt::WA_DeleteOnClose, true);
        ocApp()->ensureVisible();
        _ignoreEditor->open();
    }
}

void SettingsView::reloadConfig()
{
    _ui->syncHiddenFilesCheckBox->setChecked(!FolderMan::instance()->ignoreHiddenFiles());
    _ui->moveToTrashCheckBox->setChecked(FolderMan::instance()->moveToTrash());
    if (Utility::hasSystemLaunchOnStartup(Theme::instance()->appName())) {
        _ui->autostartCheckBox->setChecked(true);
        _ui->autostartCheckBox->setDisabled(true);
        _ui->autostartCheckBox->setToolTip(tr("You cannot disable autostart because system-wide autostart is enabled."));
    } else {
        const bool hasAutoStart = Utility::hasLaunchOnStartup(Theme::instance()->appName());
        // make sure the binary location is correctly set
        slotToggleLaunchOnStartup(hasAutoStart);
        _ui->autostartCheckBox->setChecked(hasAutoStart);
        connect(_ui->autostartCheckBox, &QAbstractButton::toggled, this, &SettingsView::slotToggleLaunchOnStartup);
    }
}

void SettingsView::loadLanguageNamesIntoDropdown()
{
    // allow method to be called more than once
    _ui->languageDropdown->clear();

    // if no option has been chosen explicitly by the user, the first entry shall be used
    _ui->languageDropdown->addItem(tr("(use default)"));

    // initialize map of locales to language names
    const auto availableLocales = []() {
        auto rv = Translations::listAvailableTranslations().values();
        rv.sort(Qt::CaseInsensitive);
        return rv;
    }();

    for (const auto &availableLocale : availableLocales) {
        auto nativeLanguageName = QLocale(availableLocale).nativeLanguageName();

        // fallback if there's a locale whose name Qt doesn't know
        // this indicates a broken filename
        if (nativeLanguageName.isEmpty()) {
            qCDebug(lcApplication()) << "Warning: could not find native language name for locale" << availableLocale;
            nativeLanguageName = tr("unknown (%1)").arg(availableLocale);
        }

        QString entryText = QStringLiteral("%1 (%2)").arg(nativeLanguageName, availableLocale);
        _ui->languageDropdown->addItem(entryText, availableLocale);
    }
}

void SettingsView::slotShowLogSettings()
{
    auto logBrowser = new LogBrowser(ocApp()->mainWindow());
    logBrowser->setAttribute(Qt::WA_DeleteOnClose);
    ocApp()->ensureVisible();
    logBrowser->open();
}

} // namespace OCC
