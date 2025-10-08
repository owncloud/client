/*
 * Copyright (C) by Olivier Goffart <ogoffart@woboq.com>
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

#include "syncoptions.h"
#include "common/utility.h"

#include <QRegularExpression>

using namespace OCC;

SyncOptions::SyncOptions(QSharedPointer<Vfs> vfs)
    : _vfs(vfs)
{
}

SyncOptions::~SyncOptions()
{
}

void SyncOptions::fillFromEnvironmentVariables()
{
    int maxParallel = qEnvironmentVariableIntValue("OWNCLOUD_MAX_PARALLEL");
    if (maxParallel > 0)
        _parallelNetworkJobs = maxParallel;
}

QRegularExpression SyncOptions::fileRegex() const
{
    return _fileRegex;
}

void SyncOptions::setFilePattern(const QString &pattern)
{
    // full match or a path ending with this pattern
    setPathPattern(QStringLiteral("(^|/|\\\\)") + pattern + QLatin1Char('$'));
}

void SyncOptions::setPathPattern(const QString &pattern)
{
    _fileRegex.setPatternOptions(
        Utility::fsCasePreservingButCaseInsensitive() ? QRegularExpression::CaseInsensitiveOption : QRegularExpression::NoPatternOption);
    _fileRegex.setPattern(pattern);
}
