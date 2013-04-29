
#ifndef MIRALL_OAUTH_OAUTHERROR_H
#define MIRALL_OAUTH_OAUTHERROR_H

enum OAuthError {
    AuthorizationNotGranted,
    AuthorizationTokenFail,
    ListenerCouldntStart,
    ListenerTimeout,
    TokenFetchError,
    TokenParseError
};

#endif // MIRALL_OAUTH_OAUTHERROR_H
