/*
 * Copyright (C) by Olivier Goffart <ogoffart@owncloud.com>
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
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

#include "guinetworkjobfactory.h"

/** \file guinetworkjobfactory.cpp
 *
 * \brief Class that creates GUI network jobs on demand.
 * Creates an normal or encrypted network job based on context.
 *
 * Overview
 * --------
 *
 * TODO
 *
 */

namespace OCC {

/**
 * @brief NetworkJobFactory::createThumbnailJob
 * Creates either a normal or an encrypted ThumbnailJob network job, depending on path.
 * @param path : Used to decide whether to create an encrypted or normal version of the network job,
 * and passed as is to the relevant constructor
 * @param account : passed as is to the relevant constructor
 * @param parent : passed as is to the relevant constructor
 * @return A pointer to the created network job. Will be either normal or encrypted.
 */
ThumbnailJob* GUINetworkJobFactory::createThumbnailJob(const QString& path, AccountPtr account, QObject* parent)
{
    if (! helper.isFileEncrypted(path))
        return new ThumbnailJob(path, account, parent);
    else
        // for now isFileEncrypted returns always false so this is never executed.
        // TODO orion : update once the encrypted class is created.
        return 0;
}

}
