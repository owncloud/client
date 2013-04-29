
#include "oauthlistener.h"

// qt
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QRegExp>
#include <QMap>
#include <QList>
#include <QPointer>

#include <QDebug>

typedef QMap< int, QString > ReasonPhraseMap;

const ReasonPhraseMap& reasonPhraseMap()
{
    static ReasonPhraseMap static_rpm;
    if ( static_rpm.isEmpty() )
    {
        static_rpm[ 200 ] = "OK";
        static_rpm[ 406 ] = "Not Acceptable";
        static_rpm[ 408 ] = "Request Time-Out";
    }

    return static_rpm;
}

/// keep track of connections
class ConnectionHandler : public QObject
{
    Q_OBJECT

public:
    ConnectionHandler( QTcpSocket* c, int t )
        : connection( c ),
          codeFound( false )
    {
        qDebug() << Q_FUNC_INFO << "@" << (void*)this << "connection @" << (void*)c << t;
        // listen for data
        connect( connection.data(), SIGNAL( readyRead() ), this, SLOT( slotReadyRead() ) );

        // ensure we can timeout
        connect( &timeout, SIGNAL( timeout() ), this, SLOT( slotTimeout() ) );
        timeout.setSingleShot( true );
        timeout.start( t );
    }

    ~ConnectionHandler()
    {
        qDebug() << Q_FUNC_INFO << "@" << (void*)this;
        shutdown();
    }

    void shutdown()
    {
        qDebug() << Q_FUNC_INFO;
        timeout.stop();
        if ( connection )
            connection->close();
    }

    QTimer timeout;
    QPointer< QTcpSocket > connection;
    QByteArray buffer;
    bool codeFound;

private slots:
    void slotTimeout()
    {
        qDebug() << Q_FUNC_INFO;

        writeErrorResponse();
        emit error( CodeTimeout );
    }

    void slotReadyRead()
    {
        if ( codeFound )
            return;

        buffer += connection->readAll();
        qDebug() << Q_FUNC_INFO << "@" << (void*)this << buffer;

        QRegExp re( ".*code=(\\S+)\\s.*", Qt::CaseInsensitive );
        if ( re.exactMatch( buffer ) )
        {
            codeFound = true;
            QString code = re.cap( 1 );
            emit codeReceived( code );
            writeResponse();
            return;
        }

        if ( connection->bytesAvailable() )
            QTimer::singleShot( 0, this, SLOT( slotReadyRead() ) );
    }

private:
    QByteArray createResponse( int code = 200, const QByteArray& payload = QByteArray() ) const
    {
        QByteArray header;
        header += QString( "HTTP/1.1 %1 %2\r\n" ).arg( code ).arg( reasonPhraseMap()[ code ] ).toUtf8();
        header += QString( "Content-Length: %1\r\n" ).arg( payload.size() ).toUtf8();
        
        if ( payload.size() )
            header += "\r\n" + payload;

        return header;
    }

    void writeResponse()
    {
        Q_ASSERT( connection );
        QByteArray body = 
            "<html><body>\n"
            "code received!"
            "</body></html>\n";

        QByteArray response = createResponse( 200, body );
        connection->write( response );
        connection->flush();
    }

    void writeErrorResponse()
    {
        Q_ASSERT( connection );
        QByteArray body = 
            "<html><body>\n"
            "failed to get code!"
            "</body></html>\n";

        /// \todo" change code to 408 to get client to resend if there 
        /// really is a timeout
        QByteArray response = createResponse( 406, body ); 
        connection->write( response );
        connection->flush();
    }

signals:
    void error( OAuthListenerError );
    void codeReceived( const QString& );
};


/// impl
class OAuthListener::OAuthListenerPrivate : public QObject
{
    Q_OBJECT

public:
    OAuthListenerPrivate( OAuthListener* o )
        : QObject( o ),
          server( new QTcpServer( this ) ),
          timeout( 2000 )
    {
        // pass signals up the chain
        connect( this, SIGNAL( codeReceived( const QString& ) ), o, SIGNAL( codeReceived( const QString& ) ) );
        connect( this, SIGNAL( error( OAuthListenerError ) ), o, SIGNAL( error( OAuthListenerError ) ) );

        // listen for connections
        connect( server, SIGNAL( newConnection() ), this, SLOT( slotNewConnection() ) );

        // start the server
        if ( !server->listen( QHostAddress::LocalHost ) )
        {
            qWarning() << Q_FUNC_INFO << "failed to start server";
            emit error( CouldntStartServer );
            return;
        }

        qDebug() << Q_FUNC_INFO << "started listening socket";
    }

    ~OAuthListenerPrivate()
    {
        qDebug() << Q_FUNC_INFO;

        foreach( ConnectionHandler* c, connections )
            c->deleteLater();
    }

    QTcpServer* server;
    QList< ConnectionHandler* > connections;
    int timeout;

private slots:
    void slotNewConnection()
    {
        qDebug() << Q_FUNC_INFO;
        ConnectionHandler* handler = new ConnectionHandler( server->nextPendingConnection(), timeout );
        connections.push_back( handler );

        connect( handler, SIGNAL( codeReceived( const QString& ) ), this, SIGNAL( codeReceived( const QString& ) ) );
        connect( handler, SIGNAL( error( OAuthListenerError ) ), this, SIGNAL( error( OAuthListenerError ) ) );        
    }

signals:
    void error( OAuthListenerError );
    void codeReceived( const QString& );
};

#include "oauthlistener.moc"

///
OAuthListener::OAuthListener( QObject* parent )
    : m_impl( new OAuthListenerPrivate( this ) )
{
    qDebug() << Q_FUNC_INFO;
}

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
    return m_impl->timeout;
}

void OAuthListener::setTimeout( int ms )
{
    m_impl->timeout = ms;
}

quint16 OAuthListener::port() const
{
    return m_impl->server->serverPort();
}

