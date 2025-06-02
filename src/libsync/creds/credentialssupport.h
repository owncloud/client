#pragma once

#include <QNetworkRequest>

namespace OCC {

inline constexpr QNetworkRequest::Attribute DontAddCredentialsAttribute = QNetworkRequest::User;

enum class AuthenticationType { Unknown = 0, OAuth };
}
