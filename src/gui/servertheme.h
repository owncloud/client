#pragma once

#include "abstractcorejob.h"
#include "accessmanager.h"
#include "networkjobs/jsonjob.h"

namespace OCC {
class ServerThemeResourcesManager;

//
//class OWNCLOUDSYNC_EXPORT ServerThemeResourcesManager : public QObject
//{
//    Q_OBJECT
//
//public:
//    explicit ServerThemeResourcesManager(AccountPtr accountPtr, QObject *parent = nullptr);
//
//    CoreJob *get(const QString &resourceUrlPath, QObject *parent = nullptr);
//
//private:
//    AccountPtr _accountPtr;
//    QAbstractNetworkCache *_cache;
//    AccessManager *_accessManager;
//};


class OWNCLOUDSYNC_EXPORT ServerTheme
{
    // merely for the enum
    Q_GADGET

public:
    enum class Type {
        Default,
        Light = Default,

        DefaultDark,
        Dark = DefaultDark,

        Count,
    };
    Q_ENUM(Type)

    ServerTheme();
    explicit ServerTheme(const QJsonObject &object);

    /**
     * Try to find icon matching provided name:
     *  - first, we check for an icon in the preferred theme type
     *  - if none is available, we look for the default
     *  - if none is available, we fall back to the common endpoint
     * Please note that the URL is not tested, i.e., we don't guarantee a resource is available at the assumed location.
     * @param iconName string identifying the icon on the server
     * @param themeType preferred theme type
     * @return path to the resource if URL lookup yielded a result, empty QUrl otherwise
     */
    QString iconPath(const QString &iconName, Type themeType = Type::Default);

    [[nodiscard]] bool isEmpty() const;

private:
    QJsonObject _object;
};


class OWNCLOUDSYNC_EXPORT ServerThemeJob : public JsonJob
{
public:
    ServerThemeJob(AccountPtr account, const QString &themeName, QObject *parent = nullptr);

    [[nodiscard]] ServerTheme serverTheme() const;
};


} // OCC::ThemeJobs

//Q_DECLARE_METATYPE(OCC::ServerTheme)
