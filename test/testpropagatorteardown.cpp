/*
 *    This software is in the public domain, furnished "as is", without technical
 *    support, and with no warranty, express or implied, as to its usefulness for
 *    any purpose.
 *
 */

#include <QtTest>
#include "testutils/syncenginetestutils.h"
#include <syncengine.h>

using namespace OCC;


class TestPropagatorTeardown : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // Destroying the engine while a download is still running must not touch the propagator's destroyed members.
    // A regression only fails this test in a build with AddressSanitizer.
    void testDestroyEngineDuringDownload()
    {
        auto fakeFolder = std::make_unique<FakeFolder>(FileInfo{});
        fakeFolder->remoteModifier().mkdir(QStringLiteral("A"));
        fakeFolder->remoteModifier().insert(QStringLiteral("A/a1"));

        bool downloadStarted = false;
        fakeFolder->setServerOverride([&downloadStarted, fakeFolder = fakeFolder.get()](QNetworkAccessManager::Operation op,
                                          const QNetworkRequest &request, QIODevice *) -> QNetworkReply * {
            if (op == QNetworkAccessManager::GetOperation) {
                downloadStarted = true;
                return new FakeHangingReply(op, request, fakeFolder);
            }
            return nullptr;
        });

        fakeFolder->scheduleSync();
        QTRY_VERIFY(downloadStarted);

        // the download job is still in the propagator's _activeJobList when the propagator is deleted
        fakeFolder.reset();
    }
};

QTEST_GUILESS_MAIN(TestPropagatorTeardown)
#include "testpropagatorteardown.moc"
