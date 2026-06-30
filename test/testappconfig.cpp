// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>

#include "libsync/owncloudtheme.h"
#include "libsync/config/appconfig.h"

#include <QSettings>
#include <QTemporaryDir>

class TestAppConfig : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testConfigPath()
    {
        auto t = OCC::ownCloudTheme();
        QCOMPARE(OCC::AppConfig::configPath(QOperatingSystemVersion::Windows, t), QString("HKEY_LOCAL_MACHINE\\Software\\Policies\\ownCloud\\ownCloud"));
        QCOMPARE(OCC::AppConfig::configPath(QOperatingSystemVersion::MacOS, t), QString("/Library/Preferences/com.owncloud.desktopclient/ownCloud.ini"));
        QCOMPARE(OCC::AppConfig::configPath(QOperatingSystemVersion::Unknown, t), QString("/etc/ownCloud/ownCloud.ini"));
    }

    void testLoadOpenIdConfig()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("system.ini"));

        {
            QSettings settings(path, QSettings::IniFormat);
            settings.setValue(QStringLiteral("OpenIDConnect/ClientId"), QStringLiteral("my-client-id"));
            settings.setValue(QStringLiteral("OpenIDConnect/ClientSecret"), QStringLiteral("my-client-secret"));
            settings.setValue(QStringLiteral("OpenIDConnect/Ports"), QStringLiteral("8080,8443"));
            settings.setValue(QStringLiteral("OpenIDConnect/Scopes"), QStringLiteral("openid email"));
            settings.setValue(QStringLiteral("OpenIDConnect/Prompt"), QStringLiteral("select_account consent"));
            settings.sync();
        }

        // Drive a real AppConfig instance through its public interface, exactly as production code does.
        const QSettings settings(path, QSettings::IniFormat);
        const OCC::OpenIdConfig cfg = OCC::AppConfig(settings).openIdConfig();

        // Prompt must be read from the Prompt key, not the Ports key (regression test for #12557)
        QCOMPARE(cfg.prompt(), QStringLiteral("select_account consent"));
        QCOMPARE(cfg.ports(), (QList<quint16>{8080, 8443}));
        QCOMPARE(cfg.scopes(), QStringLiteral("openid email"));
        QCOMPARE(cfg.clientId(), QStringLiteral("my-client-id"));
        QCOMPARE(cfg.clientSecret(), QStringLiteral("my-client-secret"));
    }
};

QTEST_GUILESS_MAIN(TestAppConfig)
#include "testappconfig.moc"
