/*
 *    This software is in the public domain, furnished "as is", without technical
 *       support, and with no warranty, express or implied, as to its usefulness for
 *          any purpose.
 *          */

#include <QtTest>

#include "activitydata.h"
#include "common/utility.h"

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
        QVERIFY(a._link.host() == "owncloud.org");
    
        Activity b;
        QString link2("Linktext http://owncloud.org");
        b.extractLink(link2);
        QVERIFY(b._link.host() == "owncloud.org");
        QVERIFY(b._linkText == "Linktext" );
    }
    void testLinkSplitText() {
        Activity a;
        QString link("Link Text http://owncloud.org");
        a.extractLink(link);
        QVERIFY(a._link.host() == "owncloud.org");
        QVERIFY(a._linkText == "Link Text" );
    }
    void testLinkEncoding() {
        Activity a;
        QString link("http://owncloud.org?foo=bar");
        a.extractLink(link);
        QVERIFY(a._link.host() == "owncloud.org");
        QVERIFY(a._link.query() == "foo=bar");
        
        Activity b;
        b.extractLink("http://owncloud.org?foo=bar%20baz");
        QVERIFY(b._link.host() == "owncloud.org");
        QVERIFY(b._link.query(QUrl::EncodeSpaces) == "foo=bar%20baz");

        Activity c;
        c.extractLink("дру́жба http://owncloud.org?foo=bar%20baz");
        QVERIFY(c._link.host() == "owncloud.org");
        QVERIFY(c._link.query(QUrl::EncodeSpaces) == "foo=bar%20baz");
        QVERIFY(c._linkText == "дру́жба" );
    }
};

QTEST_APPLESS_MAIN(TestActivityData)
#include "testactivitydata.moc"
