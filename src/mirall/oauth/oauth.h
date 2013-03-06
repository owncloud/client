
#pragma once

// qt
#include <QObject>
#include <QString>

class QNetworkAccessManager;

/// Encapsulation of OAuth.
class OAuth : public QObject
{
    Q_OBJECT

public:
    enum Error {
        AuthorizationTokenFail
    };

    OAuth( QObject* parent = NULL );

    QNetworkAccessManager* accessManager() const;
    void setAccessManager( QNetworkAccessManager* );

    static QString getStringForError( Error );

public slots:
    void authenticate();

signals:
    void error( Error );
    void authenticated();

private:
    class OAuthPrivate;
    OAuthPrivate* m_impl;
};
