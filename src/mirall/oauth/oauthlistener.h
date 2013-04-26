
#ifndef MIRALL_OAUTH_OAUTHLISTENER_H
#define MIRALL_OAUTH_OAUTHLISTENER_H

// qt
#include <QObject>

enum OAuthListenerError {
    CouldntStartServer,
    CodeTimeout
};

class OAuthListener : public QObject
{
    Q_OBJECT

public:
    OAuthListener( QObject* parent = NULL );

    static QString getStringForError( OAuthListenerError );

    /// timeout in ms
    int timeout() const;
    void setTimeout( int ms );

    /// find port used
    quint16 port() const;

signals:
    void error( OAuthListenerError );
    void codeReceived( const QString& );

private:
    class OAuthListenerPrivate;
    OAuthListenerPrivate* m_impl;
};

#endif // MIRALL_OAUTH_OAUTHLISTENER_H
