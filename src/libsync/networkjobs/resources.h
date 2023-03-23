#pragma once

#include "account.h"
#include "networkjobs.h"

#include <QIcon>
#include <QLoggingCategory>

namespace OCC {

class ResourcesCache;

/**
 * This job automatically downloads all available data from the server and stores it in a temporary cache directory on the disk.
 * For convenience, a couple of conversion functions are available to convert the binary data to common Qt classes such as QIcon.
 */
class OCSYNC_EXPORT ResourceJob : public SimpleNetworkJob
{
public:
    void finished() override;

    QIcon asIcon() const;

    const QByteArray &data() const;

protected:
    explicit ResourceJob(const ResourcesCache *cache, const QUrl &rootUrl, const QString &path, QObject *parent = nullptr);
    ;

private:
    const ResourcesCache *_cache;
    QByteArray _data;
    QString _cacheKey;

    friend class ResourcesCache;
};


class OCSYNC_EXPORT ResourcesCache : public QObject
{
    Q_OBJECT

public:
    explicit ResourcesCache(Account *account);

    ResourceJob *makeGetJob(const QUrl &rootUrl, const QString &path, QObject *parent = nullptr) const;

    ResourceJob *makeGetJob(const QString &path, QObject *parent = nullptr) const;

    void setCacheDirectory(const QString &cacheDirectory);

    Account *account() const;

    QString path(const QString &cacheKey) const;

private:
    Account *_account;
    QString _cacheDirectory;
};

} // namespace OCC
