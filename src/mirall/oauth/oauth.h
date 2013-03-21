
#pragma once

// qt
#include <QObject>
#include <QString>

class QNetworkAccessManager;

enum OAuthError {
    AuthorizationTokenFail,
    ListenerCouldntStart,
    ListenerTimeout,
    TokenFetchError,
    TokenParseError
};

/// Encapsulation of OAuth.
class OAuth : public QObject
{
    Q_OBJECT

public:
    OAuth( QObject* parent = NULL );

    QNetworkAccessManager* accessManager() const;
    void setAccessManager( QNetworkAccessManager* );

    static QString getStringForError( OAuthError );

public slots:
    void authenticate( const QString& user, const QString& pass, const QString& conn );

signals:
    void error( OAuthError );
    void authenticated( const QString& user, const QString& pass, bool useOAuth, const QString& conn );

private:
    class OAuthPrivate;
    OAuthPrivate* m_impl;
};
