// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>

#include "libsync/owncloudtheme.h"
#include "libsync/config/appconfig.h"

static const std::string_view iniData{
    "[Setup]\n"
    "ServerUrl=https://cloud.example.com\n"
    "AllowServerUrlChange=false\n"
    "MoveToTrash=true\n"
    "\n"
    "[Updater]\n"
    "SkipUpdateCheck=true\n"
    "\n"
    "[OpenIDConnect]\n"
    "ClientId=your-client-id\n"
    "ClientSecret=your-client-secret\n"
    "Ports=8080,8443\n"
    "Scopes=openid offline_access email profile\n"
    "Prompt=select_account consent\n"
};

class TestAppConfig : public QObject
{
    Q_OBJECT

    QString _configPath;

    void createConfigFile() {
        auto theme = OCC::ownCloudTheme();
        _configPath = OCC::AppConfig::configPath(QOperatingSystemVersion::Unknown, theme);
    }

    void cleanupConfigFile() {
        QFile::remove(_configPath);
    }

private Q_SLOTS:
    void testConfigPath()
    {
        auto t = OCC::ownCloudTheme();
        QCOMPARE(OCC::AppConfig::configPath(QOperatingSystemVersion::Windows, t), QString("HKEY_LOCAL_MACHINE\\Software\\Policies\\ownCloud\\ownCloud"));
        QCOMPARE(OCC::AppConfig::configPath(QOperatingSystemVersion::MacOS, t), QString("/Library/Preferences/com.owncloud.desktopclient/ownCloud.ini"));
        QCOMPARE(OCC::AppConfig::configPath(QOperatingSystemVersion::Unknown, t), QString("/etc/ownCloud/ownCloud.ini"));
    }

    void testFromFile()
    {
        auto cleanup = qScopeGuard([this]() { TestAppConfig::cleanupConfigFile(); });
        createConfigFile();

        if (!OCC::Utility::isLinux()) {
            QSKIP("This test only works on Linux");
        }

        QString parent = QFileInfo(_configPath).path();
        QVERIFY(QDir().mkpath(parent));
        QFile iniFile(_configPath);
        if (!iniFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qDebug()<<iniFile.errorString();
            QVERIFY(false);
        }
        QCOMPARE(iniFile.write(QByteArray(iniData)), iniData.length());
        iniFile.close();

        OCC::AppConfig appConfig;
        QVERIFY(!appConfig.allowServerUrlChange());

        OCC::OpenIdConfig oidCfg = appConfig.openIdConfig();
        QCOMPARE(oidCfg.clientId(), "your-client-id");
        QCOMPARE(oidCfg.clientSecret(), "your-client-secret");
        qDebug()<<oidCfg.ports();
        QCOMPARE(oidCfg.ports(), (QList<quint16>{8080, 8443}));
        QCOMPARE(oidCfg.scopes(), "openid offline_access email profile");
        QCOMPARE(oidCfg.prompt(), "select_account consent");
    }
};

QTEST_GUILESS_MAIN(TestAppConfig)
#include "testappconfig.moc"
