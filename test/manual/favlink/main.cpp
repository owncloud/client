/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../../../src/libsync/utility.h"

#include <QDir>

int main(int argc, char* argv[])
{
   QString dir="/tmp/linktest/";
   QDir().mkpath(dir);
   OCC::Utility::setupFavLink(dir);
}
