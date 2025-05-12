/*
 * libcsync -- a library to sync a directory with another
 *
 * Copyright (c) 2013 by Klaas Freitag <freitag@owncloud.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "common/filesystembase.h"
#include "csync/csync.h"
#include "csync/vio/csync_vio_local.h"
#include "testutils/testutils.h"

#include <QTemporaryFile>
#include <QTest>


class TestFileSystem : public QObject
{
    Q_OBJECT

private Q_SLOTS:
#ifdef Q_OS_WIN
    void check_long_win_path()
    {
        {
            const auto path = QStringLiteral("C://DATA/FILES/MUSIC/MY_MUSIC.mp3"); // check a short path
            const auto exp_path = QStringLiteral("\\\\?\\C:\\\\DATA\\FILES\\MUSIC\\MY_MUSIC.mp3");
            QString new_short = OCC::FileSystem::longWinPath(path);
            QCOMPARE(new_short, exp_path);
        }

        {
            const auto path = QStringLiteral("\\\\foo\\bar/MY_MUSIC.mp3");
            const auto exp_path = QStringLiteral("\\\\foo\\bar\\MY_MUSIC.mp3");
            QString new_short = OCC::FileSystem::longWinPath(path);
            QCOMPARE(new_short, exp_path);
        }

        {
            const auto path = QStringLiteral("//foo\\bar/MY_MUSIC.mp3");
            const auto exp_path = QStringLiteral("\\\\foo\\bar\\MY_MUSIC.mp3");
            QString new_short = OCC::FileSystem::longWinPath(path);
            QCOMPARE(new_short, exp_path);
        }

        {
            const auto path = QStringLiteral("\\foo\\bar");
            const auto exp_path = QStringLiteral("\\\\?\\foo\\bar");
            QString new_short = OCC::FileSystem::longWinPath(path);
            QCOMPARE(new_short, exp_path);
        }

        {
            const auto path = QStringLiteral("/foo/bar");
            const auto exp_path = QStringLiteral("\\\\?\\foo\\bar");
            QString new_short = OCC::FileSystem::longWinPath(path);
            QCOMPARE(new_short, exp_path);
        }

        const auto longPath = QStringLiteral("D://alonglonglonglong/blonglonglonglong/clonglonglonglong/dlonglonglonglong/"
                                             "elonglonglonglong/flonglonglonglong/glonglonglonglong/hlonglonglonglong/ilonglonglonglong/"
                                             "jlonglonglonglong/klonglonglonglong/llonglonglonglong/mlonglonglonglong/nlonglonglonglong/"
                                             "olonglonglonglong/file.txt");
        const auto longPathConv = QStringLiteral("\\\\?\\D:\\\\alonglonglonglong\\blonglonglonglong\\clonglonglonglong\\dlonglonglonglong\\"
                                                 "elonglonglonglong\\flonglonglonglong\\glonglonglonglong\\hlonglonglonglong\\ilonglonglonglong\\"
                                                 "jlonglonglonglong\\klonglonglonglong\\llonglonglonglong\\mlonglonglonglong\\nlonglonglonglong\\"
                                                 "olonglonglonglong\\file.txt");

        QString new_long = OCC::FileSystem::longWinPath(longPath);
        // printf( "XXXXXXXXXXXX %s %d\n", new_long, mem_reserved);

        QCOMPARE(new_long, longPathConv);

        // printf( "YYYYYYYYYYYY %ld\n", strlen(new_long));
        QCOMPARE(new_long.length(), 286);
    }
#endif


    void testLongPathStat_data()
    {
        QTest::addColumn<QString>("name");

        QTest::newRow("long") << QStringLiteral("/alonglonglonglong/blonglonglonglong/clonglonglonglong/dlonglonglonglong/"
                                                "elonglonglonglong/flonglonglonglong/glonglonglonglong/hlonglonglonglong/ilonglonglonglong/"
                                                "jlonglonglonglong/klonglonglonglong/llonglonglonglong/mlonglonglonglong/nlonglonglonglong/"
                                                "olonglonglonglong/file.txt");
        QTest::newRow("long emoji") << QString::fromUtf8("/alonglonglonglong/blonglonglonglong/clonglonglonglong/dlonglonglonglong/"
                                                         "elonglonglonglong/flonglonglonglong/glonglonglonglong/hlonglonglonglong/ilonglonglonglong/"
                                                         "jlonglonglonglong/klonglonglonglong/llonglonglonglong/mlonglonglonglong/nlonglonglonglong/"
                                                         "olonglonglonglong/file🐷.txt");
        QTest::newRow("long russian") << QString::fromUtf8("/alonglonglonglong/blonglonglonglong/clonglonglonglong/dlonglonglonglong/"
                                                           "elonglonglonglong/flonglonglonglong/glonglonglonglong/hlonglonglonglong/ilonglonglonglong/"
                                                           "jlonglonglonglong/klonglonglonglong/llonglonglonglong/mlonglonglonglong/nlonglonglonglong/"
                                                           "olonglonglonglong/собственное.txt");
        QTest::newRow("long arabic") << QString::fromUtf8("/alonglonglonglong/blonglonglonglong/clonglonglonglong/dlonglonglonglong/"
                                                          "elonglonglonglong/flonglonglonglong/glonglonglonglong/hlonglonglonglong/ilonglonglonglong/"
                                                          "jlonglonglonglong/klonglonglonglong/llonglonglonglong/mlonglonglonglong/nlonglonglonglong/"
                                                          "olonglonglonglong/السحاب.txt");
        QTest::newRow("long chinese") << QString::fromUtf8("/alonglonglonglong/blonglonglonglong/clonglonglonglong/dlonglonglonglong/"
                                                           "elonglonglonglong/flonglonglonglong/glonglonglonglong/hlonglonglonglong/ilonglonglonglong/"
                                                           "jlonglonglonglong/klonglonglonglong/llonglonglonglong/mlonglonglonglong/nlonglonglonglong/"
                                                           "olonglonglonglong/自己的云.txt");
    }

    void testLongPathStat()
    {
        auto tmp = OCC::TestUtils::createTempDir();
        QFETCH(QString, name);
        const QFileInfo longPath(tmp.path() + name);

        const auto data = QByteArrayLiteral("hello");
        qDebug() << longPath;
        QVERIFY(longPath.dir().mkpath(QStringLiteral(".")));

        QFile file(longPath.filePath());
        QVERIFY(file.open(QFile::WriteOnly));
        QVERIFY(file.write(data.constData()) == data.size());
        file.close();

        csync_file_stat_t buf;
        QVERIFY(csync_vio_local_stat(longPath.filePath(), &buf) != -1);
        QVERIFY(buf.size == data.size());
        QVERIFY(buf.size == longPath.size());

        QVERIFY(tmp.remove());
    }

    void testMkdir_data()
    {
        QTest::addColumn<QString>("name");

        const unsigned char a_umlaut_composed_bytes[] = {0xc3, 0xa4, 0x00};
        const QString a_umlaut_composed = QString::fromUtf8(reinterpret_cast<const char *>(a_umlaut_composed_bytes));
        const QString a_umlaut_decomposed = a_umlaut_composed.normalized(QString::NormalizationForm_D);

        QTest::newRow("a-umlaut composed") << a_umlaut_composed;
        QTest::newRow("a-umlaut decomposed") << a_umlaut_decomposed;
    }

    // This is not a full test, it is meant to verify that no nomalization changes are done.
    // The implementation of `OCC::FileSystem::mkpath` relies on either Qt (Windows) or
    // `std::filesystem` (POSIX), which should be covered by tests of their own.
    void testMkdir()
    {
        QFETCH(QString, name);

        auto tmp = OCC::TestUtils::createTempDir();
        QVERIFY(OCC::FileSystem::mkpath(tmp.path(), name));
        csync_file_stat_t buf;
        auto dh = csync_vio_local_opendir(tmp.path());
        QVERIFY(dh);
        while (auto fs = csync_vio_local_readdir(dh, nullptr)) {
            QCOMPARE(fs->path, name);
            QCOMPARE(fs->type, ItemTypeDirectory);
        }
        csync_vio_local_closedir(dh);
    }

    void testRename_data()
    {
        QTest::addColumn<QString>("name");

        const unsigned char a_umlaut_composed_bytes[] = {0xc3, 0xa4, 0x00};
        const QString a_umlaut_composed = QString::fromUtf8(reinterpret_cast<const char *>(a_umlaut_composed_bytes));
        const QString a_umlaut_decomposed = a_umlaut_composed.normalized(QString::NormalizationForm_D);

        QTest::newRow("a-umlaut composed") << a_umlaut_composed;
        QTest::newRow("a-umlaut decomposed") << a_umlaut_decomposed;
    }

    // This is not a full test, it is meant to verify that no nomalization changes are done.
    void testRename()
    {
        QFETCH(QString, name);

        auto tmp = OCC::TestUtils::createTempDir();
        QFile f(tmp.path() + u"/abc");
        QVERIFY(f.open(QFile::WriteOnly));
        QByteArray data("abc");
        QCOMPARE(f.write(data), data.size());
        f.close();

        QString err;
        QVERIFY(OCC::FileSystem::uncheckedRenameReplace(f.fileName(), tmp.path() + u'/' + name, &err));

        csync_file_stat_t buf;
        auto dh = csync_vio_local_opendir(tmp.path());
        QVERIFY(dh);
        while (auto fs = csync_vio_local_readdir(dh, nullptr)) {
            QCOMPARE(fs->path, name);
            QCOMPARE(fs->type, ItemTypeFile);
        }
        csync_vio_local_closedir(dh);
    }
};

QTEST_GUILESS_MAIN(TestFileSystem)
#include "testfilesystem.moc"
