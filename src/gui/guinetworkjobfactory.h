/*
 * Copyright (C) by orion1024 <orion1024+src@use.startmail.com>
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

#ifndef GUINETWORKJOBFACTORY_H
#define GUINETWORKJOBFACTORY_H

#include <QObject>

#include "networkjobfactory.h"
#include "thumbnailjob.h"

namespace OCC {

/**
 * @brief Will handle reading from and writing to metadata information to permanent storage
 * @ingroup libowncrypt
 */
class GUINetworkJobFactory : public QObject {

public:

    GUINetworkJobFactory(QObject *parent=0)
        : QObject(parent) {}

    ThumbnailJob* createThumbnailJob(const QString& path, AccountPtr account, QObject* parent = 0);

    /*
     * TODO orion : can't choose between normal and encrypted at creation time with current implementation.
     * need to think about how to deal with these 2 jobs
     *
    OcsShareeJob* createOcsShareeJob(AccountPtr account);
    OcsShareJob* createOcsShareJob(AccountPtr account);
    */
private:
    NetworkJobFactoryHelper helper;
};

}

#endif
