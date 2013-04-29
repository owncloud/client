
#ifndef MIRALL_OAUTH_OAUTHLISTENER_H
#define MIRALL_OAUTH_OAUTHLISTENER_H

// qt
#include <QObject>

// oauth
#include "oautherror.h"

class OAuthListener : public QObject
{
    Q_OBJECT

public:
    OAuthListener( QObject* parent = NULL );

    /// timeout in ms
    int timeout() const;
    void setTimeout( int ms );

    /// find port used
    quint16 port() const;

signals:
    void error( OAuthError );
    void codeReceived( const QString& );

private:
    class OAuthListenerPrivate;
    OAuthListenerPrivate* m_impl;
};

#endif // MIRALL_OAUTH_OAUTHLISTENER_H
