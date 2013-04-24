
#ifndef MIRALL_OAUTH_OAUTH_H
#define MIRALL_OAUTH_OAUTH_H

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

struct OAuthConnectionData {
    QString user;
    QString token;
    QString refresh;
    QString connection;
};

/// Encapsulation of OAuth.
class OAuth : public QObject
{
    Q_OBJECT

public:
    OAuth( QObject* parent = NULL );

    /// must set a QNAM; failing to do so will result in an assert
    QNetworkAccessManager* accessManager() const;
    void setAccessManager( QNetworkAccessManager* );

    static QString getStringForError( OAuthError );

public slots:
    void authenticate( const QString& user, const QString& conn );

signals:
    void error( OAuthError );
    void authenticated( const OAuthConnectionData& );

private:
    class OAuthPrivate;
    OAuthPrivate* m_impl;
};

#endif // MIRALL_OAUTH_OAUTH_H
