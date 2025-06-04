#pragma once

#include <QObject>
#include <QUrl>

#include "capabilities.h"

class QNetworkAccessManager;

namespace OCC {

struct FetchCapabilitiesResult
{
    QString error;
    Capabilities capabilities = Capabilities(QUrl(), {});

    bool success() const { return error.isEmpty() && capabilities.isValid(); }
};

class FetchCapabilitiesAdapter : public QObject
{
    Q_OBJECT
public:
    explicit FetchCapabilitiesAdapter(QNetworkAccessManager *nam, const QString &authToken, const QUrl &url, QObject *parent = nullptr);

    FetchCapabilitiesResult getResult();

private:
    QNetworkAccessManager *_nam;
    QUrl _url;
    QString _authorizationHeader;
};

}
