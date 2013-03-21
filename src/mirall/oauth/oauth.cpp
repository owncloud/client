
#include "oauth.h"

// qt
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QHostAddress>
#include <QSharedPointer>
#include <QDesktopServices>
#include <QSslError>

#include <QDebug>

// json
#include "3rdparty/qt-json/json.h"

// local
#include "oauthlistener.h"

/// OAUTH constants
static const QString CLIENT_ID( "a52c72ab0903261fddc804682105f8" );
static const QString CLIENT_SECRET( "58459d4b1a0134abc624b6bd5249e2" );
static const QString BASE_URL( "https://ec2-23-23-37-57.compute-1.amazonaws.com" );
static const QString AUTH_URL( BASE_URL + "/oauth2/authorize" );
static const QString TOKEN_URL( BASE_URL + "/oauth2/token" );

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
          listener( NULL ),
          nam( NULL )
    {
        connect( this, SIGNAL( error( OAuthError ) ), owner, SIGNAL( error( OAuthError ) ) );
        connect( this, SIGNAL( authenticated( const QString&, const QString&, bool, const QString& ) ),
                 owner, SIGNAL( authenticated( const QString&, const QString&, bool, const QString& ) ) );
    }

    ~OAuthPrivate()
    {
        // shut down the listener
        listener->deleteLater();
        listener = NULL;
    }

    QNetworkAccessManager* accessManager() const
    {
        if ( nam.isNull() )
        {
            nam = QSharedPointer< QNetworkAccessManager >( new QNetworkAccessManager );

            /// \todo: remove this; it looks like there is a mechanism for handling and registering
            /// exceptions within mirall
            connect( nam.data(), SIGNAL( sslErrors( QNetworkReply*, QList<QSslError> ) ),
                     this, SLOT( onIgnoreSSLErrors( QNetworkReply*, QList<QSslError> ) ) );
        }

        return nam.data();
    }
    
    // this is being passed in; someone else will take care
    // of the lifetime
    void setAccessManager( QNetworkAccessManager* n )
    {
        nam = QSharedPointer< QNetworkAccessManager >( n, nullDeleter );
    }

    void authenticate( const QString& _user, const QString& _pass, const QString& _conn )
    {
        // store user, connection
        user = _user;
        conn = _conn;

        // look to see if we have a refresh token
        if ( !refreshToken.isEmpty() )
        {
            // go direct to step 2 and re-request a token
            useRefreshToken();
        }
        else
        {
            // kick off authentication
            listener = new OAuthListener( this );
            connect( listener, SIGNAL( codeReceived( const QString& ) ), this, SLOT( onCodeReceived( const QString& ) ) );
            connect( listener, SIGNAL( error( OAuthListenerError ) ), this, SLOT( onListenerError( OAuthListenerError ) ) );
            
            QUrl auth( AUTH_URL );
            auth.addQueryItem( "response_type", "code" );
            auth.addQueryItem( "client_id", CLIENT_ID );
            auth.addEncodedQueryItem( "redirect_uri", QString( "http://localhost:%1" ).arg( listener->port() ).toUtf8().toPercentEncoding() );
            qDebug() << Q_FUNC_INFO << auth.toString();
            
            QDesktopServices::openUrl( auth );
        }
    }

    OAuth* owner;
    OAuthListener* listener;
    QString user;
    QString pass;
    QString conn;
    QString refreshToken;

signals:
    void error( OAuthError );
    void authenticated( const QString& user, const QString& pass, bool useOAuth, const QString& conn );

private slots:
    void onIgnoreSSLErrors( QNetworkReply* reply, QList<QSslError> error )
    {
        reply->ignoreSslErrors( error );
    }

    void onListenerError( OAuthListenerError e )
    {
        Q_UNUSED( e );
    }

    void onCodeReceived( const QString& code )
    {
        // should be here once we receive a code; we can now use this to get access and refresh tokens
        qDebug() << Q_FUNC_INFO << code;

        // construct payload for token request
        QByteArray payload;
        payload += "code=" + code + "&";
        payload += "client_id=" + CLIENT_ID + "&";
        payload += "client_secret=" + CLIENT_SECRET + "&";
        payload += "redirect_uri=" + QString( "http://localhost:%1" ).arg( listener->port() ) +  "&";
        payload += "grant_type=authorization_code";

        qDebug() << Q_FUNC_INFO << payload;
        
        QNetworkRequest request( TOKEN_URL );
        request.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );
        QNetworkReply* post = accessManager()->post( request, payload );
        connect( post, SIGNAL( error( QNetworkReply::NetworkError ) ), this, SLOT( onTokenError( QNetworkReply::NetworkError ) ) );
        connect( post, SIGNAL( finished() ), this, SLOT( onTokenFinished() ) );
    }

    void useRefreshToken()
    {
        // construct payload for token request
        QByteArray payload;
        payload += "client_id=" + CLIENT_ID + "&";
        payload += "client_secret=" + CLIENT_SECRET + "&";
        payload += "refresh_token=" + refreshToken +  "&";
        payload += "grant_type=refresh_token";

        qDebug() << Q_FUNC_INFO << payload;
        
        QNetworkRequest request( TOKEN_URL );
        request.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );
        QNetworkReply* post = accessManager()->post( request, payload );
        connect( post, SIGNAL( error( QNetworkReply::NetworkError ) ), this, SLOT( onTokenError( QNetworkReply::NetworkError ) ) );
        connect( post, SIGNAL( finished() ), this, SLOT( onTokenFinished() ) );
    }

    void onTokenError( QNetworkReply::NetworkError e )
    {
        qWarning() << Q_FUNC_INFO << e;
        QNetworkReply* reply = static_cast< QNetworkReply* >( sender() );
        QByteArray data = reply->readAll();

        // extract any error information if present
        if ( !data.isEmpty() )
        {
            QVariantMap m = QtJson::parse( data ).toMap();
            if ( m.contains( QLatin1String( "error" ) ) )
                qWarning() << m[ QLatin1String( "error" ) ].toString() << m[ QLatin1String( "error_description" ) ].toString();
        }

        reply->deleteLater();
        emit error( TokenFetchError );
    }

    void onTokenFinished()
    {
        QNetworkReply* reply = static_cast< QNetworkReply* >( sender() );
        QByteArray json = reply->readAll();
        qDebug() << Q_FUNC_INFO << json;

        // parse, return token as password
        // reply should be either:
        // access:  {"access_token": "8b9e41fdfd", "token_type": "bearer", "expires_in": 3600, "refresh_token": "b779561920", "scope": ""}
        // refresh: {"access_token": "8b9e41fdfd", "token_type": "bearer", "expires_in": 3600}
        QVariantMap m =QtJson::parse( json ).toMap();
        if ( m.isEmpty() )
        {
            qWarning() << Q_FUNC_INFO << "no data in map";
            emit error( TokenParseError );
            return;
        }

        QString _accessToken;
        QString _refreshToken;
        
        _accessToken = m[ QLatin1String( "access_token" ) ].toString();
        if ( m.contains( QLatin1String( "refresh_token" ) ) )
            _refreshToken = m[ QLatin1String( "refresh_token" ) ].toString();

        if ( _accessToken.isEmpty() || _refreshToken.isEmpty() )
        {
            qWarning() << Q_FUNC_INFO << "failed to get access or refresh token";
            emit error( TokenParseError );
            return;
        }
        
        pass = _accessToken;
        refreshToken = _refreshToken;

        // success!
        emit authenticated( user, pass, true, conn );
    }

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

QString OAuth::getStringForError( OAuthError e )
{
    switch( e )
    {
    case AuthorizationTokenFail: return tr( "Failed to get authorization token" );
    case ListenerCouldntStart:   return OAuthListener::getStringForError( CouldntStartServer );
    case ListenerTimeout:        return OAuthListener::getStringForError( CodeTimeout );
    case TokenFetchError:        return tr( "Failed to get token from server" );
    case TokenParseError:        return tr( "Token data returned from server is invalid" );
    default:
        break;
    }

    Q_ASSERT( false );
    return QString();
}

void OAuth::authenticate( const QString& user, const QString& pass, const QString& conn )
{
    m_impl->authenticate( user, pass, conn );
}
