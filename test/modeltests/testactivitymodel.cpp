
/*
 *    This software is in the public domain, furnished "as is", without technical
 *    support, and with no warranty, express or implied, as to its usefulness for
 *    any purpose.
 *
 */

#include "gui/models/activitylistmodel.h"
#include "gui/accountmanager.h"

#include "testutils/testutils.h"

#include <QAbstractItemModelTester>
#include <QTest>
#include <syncenginetestutils.h>

namespace OCC {

class TestActivityModel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testInsert()
    {
        auto model = new ActivityListModel(this);

        new QAbstractItemModelTester(model, this);

        //      auto fakeAm = new FakeAM(FileInfo(), this);
        auto acc1 = createDummyAccount();
        auto acc2 = createDummyAccount();

        /*    auto creds0 = new FakeCredentials(acc1->account().get(), fakeAm);
            acc1->account()->setCredentials(creds0);
            auto creds1 = new FakeCredentials(acc2->account().get(), fakeAm);
            acc2->account()->setCredentials(creds1);*/

        model->setActivityList({
            Activity{Activity::ActivityType, QStringLiteral("1"), acc1->account(), QStringLiteral("test"), QStringLiteral("test"), QStringLiteral("foo.cpp"),
                QUrl(QStringLiteral("https://owncloud.com")), QDateTime::currentDateTime()},
            Activity{Activity::ActivityType, QStringLiteral("2"), acc1->account(), QStringLiteral("test"), QStringLiteral("test"), QStringLiteral("foo.cpp"),
                QUrl(QStringLiteral("https://owncloud.com")), QDateTime::currentDateTime()},
            Activity{Activity::ActivityType, QStringLiteral("021ad48a-80ae-4af6-b878-aeb836bd367d"), acc2->account(), QStringLiteral("test"),
                QStringLiteral("test"), QStringLiteral("foo.cpp"), QUrl(QStringLiteral("https://owncloud.com")), QDateTime::currentDateTime()},
        });
        model->setActivityList({
            Activity{Activity::ActivityType, QStringLiteral("1"), acc2->account(), QStringLiteral("test"), QStringLiteral("test"), QStringLiteral("foo.cpp"),
                QUrl(QStringLiteral("https://owncloud.com")), QDateTime::currentDateTime()},
            Activity{Activity::ActivityType, QStringLiteral("2"), acc1->account(), QStringLiteral("test"), QStringLiteral("test"), QStringLiteral("foo.cpp"),
                QUrl(QStringLiteral("https://owncloud.com")), QDateTime::currentDateTime()},
            Activity{Activity::ActivityType, QStringLiteral("021ad48a-80ae-4af6-b878-aeb836bd367d"), acc2->account(), QStringLiteral("test"),
                QStringLiteral("test"), QStringLiteral("foo.cpp"), QUrl(QStringLiteral("https://owncloud.com")), QDateTime::currentDateTime()},
        });
        model->slotRemoveAccount(AccountManager::instance()->accounts().first());
    }
};
}

QTEST_GUILESS_MAIN(OCC::TestActivityModel)
#include "testactivitymodel.moc"
