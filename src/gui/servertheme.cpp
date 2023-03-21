#include "servertheme.h"
#include "account.h"

#include <QDebug>
#include <QNetworkDiskCache>

namespace OCC {

Q_LOGGING_CATEGORY(lcServerTheme, "gui.servertheme", QtInfoMsg)

namespace {
    QString themeTypeToString(ServerTheme::Type themeType)
    {
        switch (themeType) {
        case ServerTheme::Type::Default:
            return QStringLiteral("default");
        case ServerTheme::Type::DefaultDark:
            return QStringLiteral("default-dark");
        default:
            break;
        }

        Q_UNREACHABLE();
    }

    enum class ThemeSection {
        Common,
        Desktop,

        Count,
    };

    QString themeSectionToString(ThemeSection themeSection)
    {
        switch (themeSection) {
        case ThemeSection::Common:
            return QStringLiteral("common");
        case ThemeSection::Desktop:
            return QStringLiteral("desktop");
        default:
            break;
        }

        Q_UNREACHABLE();
    }
}

QString ServerTheme::iconPath(const QString &iconName, Type themeType)
{
    qDebug() << QJsonDocument(_object).toJson();

    // look up "backwards", from the most to the least preferred icon
    // this requires both enums to begin at 0 and make no "jumps" internally
    for (int themeSectionIdx = static_cast<int>(ThemeSection::Count) - 1; themeSectionIdx >= 0; --themeSectionIdx) {
        const QString themeSection = themeSectionToString(static_cast<ThemeSection>(themeSectionIdx));

        for (int themeTypeIdx = static_cast<int>(themeType); themeTypeIdx >= 0; --themeTypeIdx) {
            const QString themeType = themeTypeToString(static_cast<Type>(themeTypeIdx));

//            const auto themeSectionObject = _document.object().value(themeSection);
//            if (themeSectionObject.isNull()) {
//                continue;
//            }
//
//            const auto themeTypeObject = themeSectionObject.toObject().value(themeType);
//            if (themeTypeObject.isNull()) {
//                continue;
//            }
//
//            const auto iconObject = themeTypeObject.toObject().value(QStringLiteral("logo"));
//            if (iconObject.isNull()) {
//                continue;
//            }
//
//            const auto iconPath = iconObject.toObject().value(iconName);
//            if (iconPath.isNull()) {
//                continue;
//            }

//            const QString iconPathStr = iconPath.toString();
            const QString iconPathStr = _object.value(themeSection).toObject().value(themeType).toObject().value(QStringLiteral("logo")).toObject().value(iconName).toString();

            if (!iconPathStr.isEmpty()) {
                qCDebug(lcServerTheme) << "Found icon for theme" << themeType << "and icon name" << iconName << "at URL" << iconPathStr;
                return iconPathStr;
            }
        }
    }

    qCWarning(lcServerTheme) << "Could not find suitable icon for theme type" << themeType << "and icon name" << iconName;

    // apparently no such icon could found in the server theme response :(
    return {};
}

ServerTheme::ServerTheme() { }

ServerTheme::ServerTheme(const QJsonObject &object)
    : _object(object)
{
}

bool ServerTheme::isEmpty() const {
    return _object.isEmpty();
}

//ServerThemeResourcesManager::ServerThemeResourcesManager(AccountPtr accountPtr, QObject *parent)
//    : QObject(parent)
//    , _accountPtr(std::move(accountPtr))
//    , _cache(new QNetworkDiskCache(this))
//    , _accessManager(new AccessManager(this))
//{
//    // using our own access manager to cache just the server theme resources
//    _accessManager->setCustomTrustedCaCertificates(_accountPtr->approvedCerts());
//    _accessManager->setCache(_cache);
//
//    connect(_accountPtr.data(), &Account::approvedCertsChanged, _accessManager, &AccessManager::setCustomTrustedCaCertificates);
//}

ServerThemeJob::ServerThemeJob(AccountPtr account, const QString &themeName, QObject *parent)
//    : JsonJob(account, account->url(), QStringLiteral("/themes/%1/theme.json").arg(themeName), "GET", {}, {}, parent)
    : JsonJob(account, QUrl(QStringLiteral("http://localhost:5000")), QStringLiteral("/themes/%1/theme.json").arg(themeName), "GET", {}, {}, parent)
{
}

ServerTheme ServerThemeJob::serverTheme() const
{
    return ServerTheme(data());
}

//ServerThemeResourceJob::ServerThemeResourceJob(AccessManager *am, const QUrl &resourceUrl, QObject *parent) {
//
//}

} // ThemeJobs
