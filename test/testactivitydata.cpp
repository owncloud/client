/*
 *    This software is in the public domain, furnished "as is", without technical
 *    support, and with no warranty, express or implied, as to its usefulness for
 *    any purpose.
 */

#include <QtTest>

#include "activitydata.h"
#include "common/utility.h"

#define LS(D) QLatin1String(D)

using namespace OCC;

class TestActivityData: public QObject
{
    Q_OBJECT

private:
  
private slots:
  
    void testLinkSplitEmpty() {
        Activity a;
        QString link;
        a.extractLink(link);
        QVERIFY(a._link.isEmpty());
    }
    void testLinkSplitSimple() {
        Activity a;
        QString link("http://owncloud.org");
        a.extractLink(link);
        QCOMPARE(a._link.host(), LS("owncloud.org"));
    
        Activity b;
        QString link2("Linktext http://owncloud.org");
        b.extractLink(link2);
        QCOMPARE(b._link.host(), LS("owncloud.org"));
        QCOMPARE(b._linkText, LS("Linktext"));
    }
    void testLinkSplitText() {
        Activity a;
        QString link("Link Text http://owncloud.org");
        a.extractLink(link);
        QCOMPARE(a._link.host(), LS("owncloud.org"));
        QCOMPARE(a._linkText, LS("Link Text"));
    }
    void testLinkEncoding() {
        Activity a;
        QString link("http://owncloud.org?foo=bar");
        a.extractLink(link);
        QCOMPARE(a._link.host(), LS("owncloud.org"));
        QCOMPARE(a._link.query(), LS("foo=bar"));
        
        Activity b;
        b.extractLink("http://owncloud.org?foo=bar%20baz");
        QCOMPARE(b._link.host(), LS("owncloud.org"));
        QCOMPARE(b._link.query(QUrl::EncodeSpaces), LS("foo=bar%20baz"));

        Activity c;
        c.extractLink("дру́жба http://owncloud.org?foo=bar%20baz");
        QCOMPARE(c._link.host(), LS("owncloud.org"));
        QCOMPARE(c._link.query(QUrl::EncodeSpaces), LS("foo=bar%20baz"));
        QCOMPARE(c._linkText,  QString::fromUtf8("дру́жба"));
    }
};

QTEST_APPLESS_MAIN(TestActivityData)
#include "testactivitydata.moc"
