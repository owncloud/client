
#include "oauthlistener.h"

// qt
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QRegExp>

#include <QDebug>

/// impl
class OAuthListener::OAuthListenerPrivate : public QObject
{
    Q_OBJECT

public:
    OAuthListenerPrivate( OAuthListener* o )
        : QObject( o ),
          server( new QTcpServer( this ) ),
          connection( NULL )
    {
        connect( this, SIGNAL( codeReceived( const QString& ) ), o, SIGNAL( codeReceived( const QString& ) ) );
        connect( this, SIGNAL( error( OAuthListenerError ) ), o, SIGNAL( error( OAuthListenerError ) ) );

        // start the server
        if ( !server->listen( QHostAddress::LocalHost ) )
        {
            qWarning() << Q_FUNC_INFO << "failed to start server";
            emit error( CouldntStartServer );
            return;
        }

        connect( server, SIGNAL( newConnection() ), this, SLOT( onNewConnection() ) );
        if ( server->hasPendingConnections() )
            onNewConnection();

        // ensure we can timeout
        connect( &timeout, SIGNAL( timeout() ), this, SLOT( onTimeout() ) );
        timeout.setSingleShot( true );
        timeout.start( 60000 ); // 1 minute timeout
    }

    QTimer timeout;
    QTcpServer* server;
    QTcpSocket* connection;
    QByteArray buffer;
    QString code;

private slots:
    void onTimeout()
    {
        emit error( CodeTimeout );
    }

    void onNewConnection()
    {
        qDebug() << Q_FUNC_INFO;
        connection = server->nextPendingConnection();
        connect( connection, SIGNAL( readyRead() ), this, SLOT( onReadyRead() ) );
    }

    void onReadyRead()
    {
        buffer += connection->readAll();

        qDebug() << Q_FUNC_INFO << buffer;

        QRegExp re( ".*code=(\\S+)\\s.*", Qt::CaseInsensitive );
        if ( re.exactMatch( buffer ) )
        {
            code = re.cap( 1 );
            timeout.stop();
            emit codeReceived( code );
        }
    }

signals:
    void error( OAuthListenerError );
    void codeReceived( const QString& );
};

#include "oauthlistener.moc"

///
OAuthListener::OAuthListener( QObject* parent )
    : m_impl( new OAuthListenerPrivate( this ) )
{}

QString OAuthListener::getStringForError( OAuthListenerError e )
{
    switch( e )
    {
    case CouldntStartServer: return tr( "Failed to start server" );
    case CodeTimeout:        return tr( "Didn't receive code within timeout period" );
    default:
        break;
    }

    Q_ASSERT( false );
    return QString();
}

int OAuthListener::timeout() const
{
    return m_impl->timeout.interval();
}

void OAuthListener::setTimeout( int ms )
{
    return m_impl->timeout.start( ms );
}

quint16 OAuthListener::port() const
{
    return m_impl->server->serverPort();
}

