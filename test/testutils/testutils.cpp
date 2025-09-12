#include "testutils.h"

#include "common/checksums.h"
#include "gui/accountmanager.h"

#include <QCoreApplication>
#include <QRandomGenerator>

namespace OCC {

namespace TestUtils {

    // We have more than one of these?
    FolderDefinition createDummyFolderDefinition(const AccountPtr &account, const QString &path)
    {
        // TODO: legacy
        auto d = OCC::FolderDefinition::createNewFolderDefinition(account->davUrl(), {});
        d.setLocalPath(path);
        d.setTargetPath(path);
        d.setJournalPath(SyncJournalDb::makeDbName(d.localPath()));
        return d;
    }

    QTemporaryDir createTempDir()
    {
        return QTemporaryDir { QStringLiteral("%1/ownCloud-unit-test-%2-XXXXXX").arg(QDir::tempPath(), qApp->applicationName()) };
    }

    FolderMan *folderMan()
    {
        static QPointer<FolderMan> man;
        if (!man) {
            man = FolderMan::createInstance().release();
            QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, man, &FolderMan::deleteLater);
        };
        return man;
    }


    bool writeRandomFile(const QString &fname, int size)
    {
        auto rg = QRandomGenerator::global();

        const int maxSize = 10 * 10 * 1024;
        if (size == -1) {
            size = static_cast<int>(rg->generate() % maxSize);
        }

        QString randString;
        for (int i = 0; i < size; i++) {
            int r = static_cast<int>(rg->generate() % 128);
            randString.append(QChar(r));
        }

        QFile file(fname);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << randString;
            // optional, as QFile destructor will already do it:
            file.close();
            return true;
        }
        return false;
    }

    const QVariantMap testCapabilities(CheckSums::Algorithm algo)
    {
        static const auto algorithmNames = [] {
            QVariantList out;
            for (const auto &a : CheckSums::All) {
                out.append(Utility::enumToString(a.first));
            }
            return out;
        }();
        // clang-format off
        return {{QStringLiteral("core"),
                    QVariantMap{{QStringLiteral("status"),
                        QVariantMap{{QStringLiteral("installed"), QStringLiteral("1")}, {QStringLiteral("maintenance"), QStringLiteral("0")},
                            {QStringLiteral("needsDbUpgrade"), QStringLiteral("0")}, {QStringLiteral("version"), QStringLiteral("10.11.0.0")},
                            {QStringLiteral("versionstring"), QStringLiteral("10.11.0")}, {QStringLiteral("edition"), QStringLiteral("Community")},
                            {QStringLiteral("productname"), QStringLiteral("Infinite Scale")}, {QStringLiteral("product"), QStringLiteral("Infinite Scale")},
                            {QStringLiteral("productversion"), QStringLiteral("2.0.0-beta1+7c2e3201b")}}}}},
            {QStringLiteral("files"), QVariantList{}},
            {QStringLiteral("spaces"), QVariantMap{{QStringLiteral("enabled"), QStringLiteral("true")}}},
            {QStringLiteral("dav"), QVariantMap{{QStringLiteral("chunking"), QStringLiteral("1.0")}}},
            {QStringLiteral("checksums"), QVariantMap{{QStringLiteral("preferredUploadType"), Utility::enumToString(algo)}, {QStringLiteral("supportedTypes"), algorithmNames}}}
        };
        // clang-format on
    }

    void TestUtilsPrivate::accountStateDeleter(OCC::AccountState *acc)
    {
        if (acc) {
            OCC::AccountManager::instance()->deleteAccount(acc);
        }
    }
}
}
