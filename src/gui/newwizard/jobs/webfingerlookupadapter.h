#pragma once

#include <QObject>
#include <QUrl>


class QNetworkAccessManager;

namespace OCC {


struct WebFingerLookupResult
{
    QString error;
    QList<QUrl> urls;

    bool success() { return error.isEmpty() && !urls.isEmpty(); }
};

class WebFingerLookupAdapter : public QObject
{
    Q_OBJECT
public:
    explicit WebFingerLookupAdapter(QNetworkAccessManager *nam, const QString &authToken, const QUrl &url, QObject *parent);

    WebFingerLookupResult getResult();

private:
    QNetworkAccessManager *_nam;
    QUrl _url;

    QString _authorizationHeader;
};
}
