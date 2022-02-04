#pragma once

#include "abstractcorejob.h"

#include "networkjobs.h"
#include "owncloudlib.h"

namespace OCC {


class OWNCLOUDSYNC_EXPORT DetermineAuthTypeJobFactory : public AbstractCoreJobFactory
{
    Q_OBJECT
public:
    using AuthType = DetermineAuthTypeJob::AuthType;

    DetermineAuthTypeJobFactory(QNetworkAccessManager *nam, QObject *parent = nullptr);
    ~DetermineAuthTypeJobFactory() override;

    Job *startJob(const QUrl &url) override;
};

}
