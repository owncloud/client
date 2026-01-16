// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtTest>

#include "testutils/testutils.h"

#include "libsync/owncloudtheme.h"
#include "libsync/config/systemconfig.h"

class TestSystemConfig : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testConfigPath()
    {
        auto t = OCC::ownCloudTheme();
        QCOMPARE(OCC::SystemConfig::configPath(QOperatingSystemVersion::Windows, t), QString("HKEY_LOCAL_MACHINE\\Software\\ownCloud\\ownCloud"));
        QCOMPARE(OCC::SystemConfig::configPath(QOperatingSystemVersion::MacOS, t), QString("/Library/Preferences/com.owncloud.desktopclient/ownCloud.ini"));
        QCOMPARE(OCC::SystemConfig::configPath(QOperatingSystemVersion::Unknown, t), QString("/etc/ownCloud/ownCloud.ini"));
    }
};

QTEST_GUILESS_MAIN(TestSystemConfig)
#include "testsystemconfig.moc"
