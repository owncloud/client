
#include "oauth.h"

// qt
#include <QNetworkAccessManager>
#include <QSharedPointer>

#include <QDebug>

static void nullDeleter( QNetworkAccessManager* )
{}

// impl
class OAuth::OAuthPrivate : public QObject
{
    Q_OBJECT

public:
    OAuthPrivate( OAuth* o )
        : QObject( o ),
          owner( o ),
          nam( NULL )
    {
        connect( this, SIGNAL( error( Error ) ), owner, SIGNAL( error( Error ) ) );
        connect( this, SIGNAL( authenticated() ), owner, SIGNAL( authenticated() ) );
    }

    QNetworkAccessManager* accessManager() const
    {
        if ( nam.isNull() )
            nam = QSharedPointer< QNetworkAccessManager >( new QNetworkAccessManager );

        return nam.data();
    }
    
    // this is being passed in; someone else will take care
    // of the lifetime
    void setAccessManager( QNetworkAccessManager* n )
    {
        nam = QSharedPointer< QNetworkAccessManager >( n, nullDeleter );
    }

    void authenticate()
    {
        // kick off authentication
        
    }
    
    OAuth* owner;

signals:
    void error( Error );
    void authenticated();

private:
    mutable QSharedPointer< QNetworkAccessManager > nam;
};

#include "oauth.moc"

///
OAuth::OAuth( QObject* parent )
    : m_impl( new OAuthPrivate( this ) )
{}

QNetworkAccessManager* OAuth::accessManager() const
{
    return m_impl->accessManager();
}

void OAuth::setAccessManager( QNetworkAccessManager* nam )
{
    m_impl->setAccessManager( nam );
}

QString OAuth::getStringForError( Error e )
{
    switch( e )
    {
    case AuthorizationTokenFail: return tr( "Failed to get authorization token" );
    default:
        break;
    }

    Q_ASSERT( false );
    return QString();
}

void OAuth::authenticate()
{
    m_impl->authenticate();
}
