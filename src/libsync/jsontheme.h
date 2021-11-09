#pragma once

#include "common/version.h"
#include "theme.h"

#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QJsonObject>
#include <QPixmap>
#include <QString>

namespace OCC {

class JsonTheme : public Theme
{
public:
    JsonTheme();

    bool isVanilla() const override
    {
        return _isVanilla;
    }

    QString defaultServerFolder() const override;

    QString quotaBaseFolder() const override;
    QString overrideServerUrl() const override;
    QString helpUrl() const override;
    QString conflictHelpUrl() const override;

    QString wizardUrlPostfix() const override;

    QColor wizardHeaderBackgroundColor() const override;
    QColor wizardHeaderTitleColor() const override;
    QString wizardUrlHint() const override;

    bool forceSystemNetworkProxy() const override;

    bool singleSyncFolder() const override;
    bool multiAccount() const override;
    bool userGroupSharing() const override;
    bool linkSharing() const override;

    UserIDType userIDType() const override;

    qint64 newBigFolderSizeLimit() const override;

    bool wizardHideExternalStorageConfirmationCheckbox() const override;
    bool wizardHideFolderSizeLimitCheckbox() const override;

    QString oauthClientId() const override;

    QString oauthClientSecret() const override;

    QString openIdConnectScopes() const override;

    QString about() const override;

    bool aboutShowCopyright() const override;

    bool showVirtualFilesOption() const override;

    bool forceVirtualFilesOption() const override;

    bool enableExperimentalFeatures() const override;

    QString customUserID() const override;

private:
    QJsonObject _data;
    QString _appNameGUI;
    QString _appName;
    bool _isVanilla;

    QJsonValue get(const QLatin1String &key) const;


    // Theme interface
public:
    QString appNameGUI() const override;
    QString appName() const override;

    QString webDavPath() const override;
    QString userIDHint() const override;
    QIcon wizardHeaderLogo() const override;

    // TODO: No key yet
    QColor wizardHeaderSubTitleColor() const override;


    // TODO: No key yet
    //aboutIcon
    //bool wizardSkipAdvancedPage() const override;
    //QString oauthLocalhost() const override;
    //QPair<QString, QString> oauthOverrideAuthUrl() const override;
    //QString openIdConnectPrompt() const override;
    //bool connectionValidatorClearCookies() const override;

    // Theme interface
public:
    virtual QString updateCheckUrl() const override;
};

} // namespace mirall
