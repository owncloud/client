
#pragma once

// qt
#include <QObject>

class OAuthListener : public QObject
{
    Q_OBJECT

public:
    enum Error {
        CouldntStartServer
    };

    OAuthListener( QObject* parent = NULL );

    static QString getStringForError( Error );

    /// timeout in ms
    int timeout() const;
    void setTimeout( int ms );

    /// find port used
    quint16 port() const;

signals:
    void error( Error );
    void codeReceived( const QString& );
    void timeout();

private:
    class OAuthListenerPrivate;
    OAuthListenerPrivate* m_impl;
};
