/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "space.h"

#include "libsync/graphapi/spacesmanager.h"
#include "libsync/networkjobs.h"
#include "libsync/networkjobs/resources.h"

#include "resources/resources.h"

using namespace OCC;
using namespace GraphApi;

namespace {

const auto personalC = QLatin1String("personal");

// https://github.com/cs3org/reva/blob/0cde0a3735beaa14ebdfd8988c3eb77b3c2ab0e6/pkg/utils/utils.go#L56-L59

// important detail about this id: the "shares" folder always has this id, even across different accounts!
// this means that we can never assume that space id's are unique across accounts, to the contrary, if a share is
// in play they most definitely are not. This concept could theoretically extend to other folders so we need to index
// space id against account id for many operations.
const auto sharesIdC = QLatin1String("a0ca6a90-a365-4782-871e-d44447bbc668$a0ca6a90-a365-4782-871e-d44447bbc668");
}

Space::Space(SpacesManager *spacesManager, const OpenAPI::OAIDrive &drive, const bool hasManyPersonalSpaces)
    : QObject(spacesManager)
    , _spaceManager(spacesManager)
    , _image(new SpaceImage(this))
    , _hasManyPersonalSpaces(hasManyPersonalSpaces)
{
    // todo future refactoring: get this setDrive out of the ctr since it potentially kicks off a job for the SpaceImage before the Space is fully constructed
    // propose removing the drive arg from the ctr completely, and moving the call to setDrive to the spacesmanager such that any call to
    // "new" space is immediately followed by setDrive.
    if (_spaceManager->account())
        _accountId = _spaceManager->account()->uuid();
    setDrive(drive);
    connect(_image, &SpaceImage::imageChanged, this, &Space::imageChanged);
}

QUuid Space::accountId() const
{
    return _accountId;
}

bool Space::setDrive(const OpenAPI::OAIDrive &drive)
{

    // first config naturally has an empty drive - reality check that updated drives are always valid
    Q_ASSERT(drive.isValid());

    QString curTag = _drive.getRoot().getETag();
    QString newTag = drive.getRoot().getETag();
    Q_ASSERT(!newTag.isEmpty());
    // the tag should change when the space is edited on the server. I verified that it changes on space rename as well
    // as on changing the space image so we may have further wrinkles if there is an error in logic server side, but for now
    // we want to reduce updates to "only when something changed" else everything is auto-refreshed periodically (eg every 30s)
    if (curTag == newTag)
        return false;

    _drive = drive;
    _image->update();
    return true;
}

QString Space::displayName() const
{
    if (_hasManyPersonalSpaces) {
        return _drive.getName();
    }

    // other systems like oCIS have one personal and one shared space and their names are hard coded
    if (_drive.getDriveType() == personalC) {
        return tr("Personal");
    }
    if (_drive.getId() == sharesIdC) {
        // don't call it ShareJail
        return tr("Shares");
    }
    return _drive.getName();
}

QString Space::description() const
{
    return _drive.getDescription();
}


uint32_t Space::priority() const
{
    if (_drive.getDriveType() == personalC) {
        return 100;
    } else if (_drive.getId() == sharesIdC) {
        return 50;
    }
    return 0;
}

bool Space::disabled() const
{
    // this is how disabled spaces are represented in the graph API
    return _drive.getRoot().getDeleted().getState() == QLatin1String("trashed");
}

SpaceImage *Space::image() const
{
    return _image;
}

QString Space::id() const
{
    return _drive.getRoot().getId();
}

QUrl Space::webDavUrl() const
{
    return QUrl(_drive.getRoot().getWebDavUrl());
}

QUrl Space::webUrl() const
{
    return QUrl(_drive.getRoot().getWebUrl());
}

QString Space::eTag() const
{
    return _drive.getETag();
}

QList<OpenAPI::OAIDriveItem> Space::getSpecialItems() const
{
    return _drive.getSpecial();
}

OpenAPI::OAIQuota Space::quota() const
{
    return _drive.getQuota();
}

SpaceImage::SpaceImage(Space *space)
    : QObject(space)
    , _space(space)
{
}

QIcon SpaceImage::image() const
{
    if (_image.isNull()) {
        return Resources::getCoreIcon(QStringLiteral("space"));
    }
    return _image;
}

QUrl SpaceImage::qmlImageUrl() const
{
    if (!_image.isNull()) {
        return QUrl(QStringLiteral("image://space/%1/%2").arg(etag(), _space->id()));
    } else {
        // invalid space id to display the placeholder
        return QUrl(QStringLiteral("image://space/placeholder"));
    }
}

void SpaceImage::update()
{
    const auto &special = _space->getSpecialItems();
    const auto img = std::find_if(special.cbegin(), special.cend(), [](const auto &it) { return it.getSpecialFolder().getName() == QLatin1String("image"); });
    if (img != special.cend())
    {
        // ssue 12057: verified the image etag does change when the space's icon has been updated via the web interface
        // check the etag before updating the members and creating the job. This should eliminate *many* pointless resource jobs which
        // will exacerbate whatever is making the app crash when the space tries to clean up it's child jobs and one is already gone
        QString newEtag = Utility::normalizeEtag(img->getETag());
        if (_etag == newEtag)
            return;

        _etag = newEtag;
        _url = QUrl(img->getWebDavUrl());

        // todo DC-150: this job should run in the spaces manager, then just set the image on this spaceImage object. We don't need all these deps floating
        // around at every level and we *definitely* don't want the spaces manager to hand out the account to whoever wants it
        auto job = _space->_spaceManager->account()->resourcesCache()->makeGetJob(_url, this);

        // TODO: next problem = this routine is correctly run when the icon has changed on the server, but the icon in the gui does not get refreshed!
        // The icon IS in the app cache, so it seems to have been brought back from the server correctly, but it is not shown until restart
        // I did have a quick look at Resources::getCoreIcon - that is used in the SpaceImage::image function. Also sus is that I can't find any slot
        // for the imageChanged signal but I think this may be connected in qml via the associated space image property.
        QObject::connect(job, &SimpleNetworkJob::finishedSignal, _space, [job, this] {
            if (job->httpStatusCode() == 200) {
                _image = job->asIcon();
                job->deleteLater();
                Q_EMIT imageChanged();
            }
        });
        job->start();
    }
}
