#include "abstractcorejob.h"

using namespace OCC;

AbstractCoreJobFactory::AbstractCoreJobFactory(QNetworkAccessManager *nam, QObject *parent)
    : QObject(parent)
    , _nam(nam)
{
}

AbstractCoreJobFactory::~AbstractCoreJobFactory()
{
}

QNetworkAccessManager *AbstractCoreJobFactory::nam() const
{
    return _nam;
}

const QVariant &Job::result() const
{
    return _result;
}

const QString &Job::error() const
{
    return _error;
}

bool Job::success() const
{
    return _success;
}

void Job::finishWithResult(const QVariant &result)
{
    _success = true;
    _result = result;
    Q_EMIT finished();
}

void Job::finishWithError(const QString &error)
{
    _error = error;
    Q_EMIT finished();
}

Job::Job(QObject *parent)
    : QObject(parent)
{
}
