/*
 * Copyright (C) by Daniel Molkentin <danimo@owncloud.com>
 * Copyright (C) by Erik Verbruggen <erik@verbruggen.consulting>
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

#include "platform_mac.h"
#include "application.h"

@interface OwnAppDelegate : NSObject <NSApplicationDelegate>
- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag;
@end

@implementation OwnAppDelegate {
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag
{
    if (auto app = qobject_cast<OCC::Application *>(QApplication::instance()))
        app->showSettingsDialog();
    return YES;
}

@end

namespace OCC {

MacPlatform::MacPlatform()
{
    NSApplicationLoad();
    _appDelegate = [[OwnAppDelegate alloc] init];
    [[NSApplication sharedApplication] setDelegate:_appDelegate];

    signal(SIGPIPE, SIG_IGN);
}

MacPlatform::~MacPlatform()
{
    [_appDelegate release];
}

void MacPlatform::migrate()
{
    Platform::migrate();

    migrateLaunchOnStartup();
}

std::unique_ptr<Platform> Platform::create()
{
    return std::make_unique<MacPlatform>();
}

} // namespace OCC
