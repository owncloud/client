/*
 *    This software is in the public domain, furnished "as is", without technical
 *    support, and with no warranty, express or implied, as to its usefulness for
 *    any purpose.
 *
 */

#include <QtTest>

#include "creds/oauth.h"
#include "networkjobs.h"

using namespace OCC;

class TestOAuthHtmlPage : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testSuccessPage() {
        const QString loginSuccessfulTitle = tr("Login Successful");
        const QString loginSuccessfulHtml = tr("<h1>Login Successful</h1><p>You can close this window.</p>");

        auto oauth = OAuth::renderHttpTemplate(loginSuccessfulTitle, loginSuccessfulHtml);

        QFile f(QStringLiteral("/home/deepdiver/Development/kiteworks/client-kiteworks/test/success.html"));
        f.open(QIODeviceBase::WriteOnly|QIODeviceBase::Truncate|QIODeviceBase::Text);
        f.write(oauth.toUtf8());
    }

    void testErrorPage() {
        const QString loginSuccessfulTitle = tr("Login Error");

        QString fieldsError;
        fieldsError.append(QStringLiteral("\tError: Missing field access_token\n"));
        auto errorReason = tr("The reply from the server did not contain all expected fields\n:%1").arg(fieldsError);
        const QString loginSuccessfulHtml = tr("<h1>Login Error</h1><p>%1</p>").arg(errorReason);

        auto oauth = OAuth::renderHttpTemplate(loginSuccessfulTitle, loginSuccessfulHtml);

        QFile f(QStringLiteral("/home/deepdiver/Development/kiteworks/client-kiteworks/test/error.html"));
        f.open(QIODeviceBase::WriteOnly|QIODeviceBase::Truncate|QIODeviceBase::Text);
        f.write(oauth.toUtf8());
    }
};

QTEST_MAIN(TestOAuthHtmlPage)
#include "testoauthhtmlpage.moc"
