#include "resources.h"

#include <QDir>
#include <QFile>
#include <QIcon>

namespace OCC {

Q_LOGGING_CATEGORY(lcResources, "sync.networkjob.resource")

namespace {
    QString hashMd5(const QString &data)
    {
        QCryptographicHash hash(QCryptographicHash::Algorithm::Md5);
        hash.addData(data.toLatin1());
        return QString::fromLatin1(hash.result().toHex());
    }

    QString cacheKey(QNetworkReply *reply)
    {
        QString urlHash = hashMd5(reply->url().toString());
        QString eTagHash = hashMd5(reply->header(QNetworkRequest::ETagHeader).toString());
        return QStringLiteral("%1-%2").arg(urlHash, eTagHash);
    }
}

void ResourceJob::finished()
{
    qCInfo(lcResources) << "Resource job of" << reply()->request().url() << "finished with status" << replyStatusString();

    if (reply()->error() != QNetworkReply::NoError) {
        qCWarning(lcResources) << "Network error: " << this << errorString();
    } else {
        _data = reply()->readAll();
        _cacheKey = cacheKey(reply());
    }

    // storing the file on disk enables Qt to apply some optimizations, e.g., in QIcon
    // also, specifically for icons, loading from disk allows easy management of scalable and non-scalable entities
    if (!_data.isEmpty()) {
        QFile cacheFile(_cache->path(_cacheKey));

        qCDebug(lcResources) << "cache file path:" << cacheFile.fileName();

        // furthermore, we can skip writing the file if the cache key has not changed (i.e., a file exists) and the file has come from the network cache
        if (cacheFile.exists() && reply()->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool()) {
            qCDebug(lcResources) << "file has come from network cache, skipping writing";
        } else {
            // note: we want to truncate the file if it exists
            if (!cacheFile.open(QIODevice::WriteOnly)) {
                qCCritical(lcResources) << "failed to open cache file for writing:" << cacheFile.fileName();
            } else {
                if (!cacheFile.write(_data)) {
                    qCCritical(lcResources) << "failed to write to cache file:" << cacheFile.fileName();
                }
            }
        }
    }

    SimpleNetworkJob::finished();
}

QIcon ResourceJob::asIcon() const
{
    // storing the file on disk enables Qt to apply some optimizations (e.g., caching of rendered pixmaps)
    Q_ASSERT(!_cacheKey.isEmpty());
    return QIcon(_cache->path(_cacheKey));
}

const QByteArray &ResourceJob::data() const
{
    return _data;
}

ResourceJob::ResourceJob(const ResourcesCache *cache, const QUrl &rootUrl, const QString &path, QObject *parent)
    : SimpleNetworkJob(cache->account()->sharedFromThis(), rootUrl, path, "GET", {}, {}, parent)
    , _cache(cache)
{
    setStoreInCache(true);
}

ResourcesCache::ResourcesCache(Account *account)
    : QObject(account)
    , _account(account)
{
}

ResourceJob *ResourcesCache::makeGetJob(const QUrl &rootUrl, const QString &path, QObject *parent) const
{
    return new ResourceJob(this, rootUrl, path, parent);
}

ResourceJob *ResourcesCache::makeGetJob(const QString &path, QObject *parent) const
{
    return makeGetJob(_account->url(), path, parent);
}

void ResourcesCache::setCacheDirectory(const QString &cacheDirectory)
{
    if (!OC_ENSURE(QDir().mkpath(cacheDirectory))) {
        qCDebug(lcResources) << "failed to create cache directory:" << cacheDirectory;
    }
    _cacheDirectory = cacheDirectory;
}

Account *ResourcesCache::account() const
{
    return _account;
}

QString ResourcesCache::path(const QString &cacheKey) const
{
    Q_ASSERT(!cacheKey.isEmpty());
    Q_ASSERT(!_cacheDirectory.isEmpty());
    return QStringLiteral("%1/%2").arg(_cacheDirectory, cacheKey);
}

}
