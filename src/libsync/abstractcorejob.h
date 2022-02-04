#pragma once

#include <QNetworkAccessManager>

#include "owncloudlib.h"

namespace OCC {

class OWNCLOUDSYNC_EXPORT Job : public QObject
{
    Q_OBJECT

    friend class AbstractCoreJobFactory;

public:
    explicit Job(QObject *parent = nullptr);

    const QVariant &result() const;

    const QString &error() const;

    bool success() const;

protected:
    void finishWithResult(const QVariant &result);

    void finishWithError(const QString &error);

Q_SIGNALS:
    void finished();

private:
    bool _success = false;
    QVariant _result;
    QString _error;
};

class OWNCLOUDSYNC_EXPORT AbstractCoreJobFactory : public QObject
{
    Q_OBJECT

public:
    AbstractCoreJobFactory(QNetworkAccessManager *nam, QObject *parent = nullptr);
    virtual ~AbstractCoreJobFactory();

    virtual Job *startJob(const QUrl &url) = 0;

protected:
    QNetworkAccessManager *nam() const;

    static void finishJobWithSuccess(Job *job, const QVariant &result)
    {
        job->finishWithResult(result);
    }

    static void finishJobWithError(Job *job, const QString &error)
    {
        job->finishWithError(error);
    }

private:
    QNetworkAccessManager *_nam;
};

}
