/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#import "SyncClientProxy.h"

@protocol ServerProtocol <NSObject>
- (void)registerClient:(id)client;
@end

@interface SyncClientProxy ()
- (void)registerTransmitter:(id)tx;
@end

@implementation SyncClientProxy

- (instancetype)initWithDelegate:(id)arg1 serverName:(NSString *)serverName
{
    self = [super init];

    self.delegate = arg1;
    _serverName = serverName;
    _remoteEnd = nil;

    return self;
}

#pragma mark - Connection setup

- (void)start
{
    if (_remoteEnd)
        return;

    // Lookup the server connection
    NSConnection *conn = [NSConnection connectionWithRegisteredName:_serverName host:nil];

    if (!conn) {
        // Could not connect to the sync client
        [self scheduleRetry];
        return;
    }

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(connectionDidDie:) name:NSConnectionDidDieNotification object:conn];

    NSDistantObject<ServerProtocol> *server = (NSDistantObject<ServerProtocol> *)[conn rootProxy];
    assert(server);

    // This saves a few Mach messages, enable "Distributed Objects" in the scheme's Run diagnostics to watch
    [server setProtocolForProxy:@protocol(ServerProtocol)];

    // Send an object to the server to act as the channel rx, we'll receive the tx through registerTransmitter
    [server registerClient:self];
}

- (void)registerTransmitter:(id)tx;
{
    // The server replied with the distant object that we will use for tx
    _remoteEnd = (NSDistantObject<ChannelProtocol> *)tx;
    [_remoteEnd setProtocolForProxy:@protocol(ChannelProtocol)];

    // Everything is set up, start querying
    [self askOnSocket:@"" query:@"GET_STRINGS"];
}

- (void)scheduleRetry
{
    [NSTimer scheduledTimerWithTimeInterval:5 target:self selector:@selector(start) userInfo:nil repeats:NO];
}

- (void)connectionDidDie:(NSNotification *)notification
{
#pragma unused(notification)
    _remoteEnd = nil;
    [_delegate connectionDidDie];

    [self scheduleRetry];
}

#pragma mark - Communication logic

- (void)sendMessage:(NSData *)msg
{
    NSString *answer = [[NSString alloc] initWithData:msg encoding:NSUTF8StringEncoding];

    // Cut the trailing newline. We always only receive one line from the client.
    answer = [answer substringToIndex:[answer length] - 1];
    NSArray *chunks = [answer componentsSeparatedByString:@":"];

    if ([[chunks objectAtIndex:0] isEqualToString:@"STATUS"]) {
        NSString *result = [chunks objectAtIndex:1];
        NSString *path = [chunks objectAtIndex:2];
        if ([chunks count] > 3) {
            for (int i = 2; i < [chunks count] - 1; i++) {
                path = [NSString stringWithFormat:@"%@:%@", path, [chunks objectAtIndex:i + 1]];
            }
        }
        [_delegate setResultForPath:path result:result];
    } else if ([[chunks objectAtIndex:0] isEqualToString:@"UPDATE_VIEW"]) {
        NSString *path = [chunks objectAtIndex:1];
        [_delegate reFetchFileNameCacheForPath:path];
    } else if ([[chunks objectAtIndex:0] isEqualToString:@"REGISTER_PATH"]) {
        NSString *path = [chunks objectAtIndex:1];
        [_delegate registerPath:path];
    } else if ([[chunks objectAtIndex:0] isEqualToString:@"UNREGISTER_PATH"]) {
        NSString *path = [chunks objectAtIndex:1];
        [_delegate unregisterPath:path];
    } else if ([[chunks objectAtIndex:0] isEqualToString:@"GET_STRINGS"]) {
        // BEGIN and END messages, do nothing.
    } else if ([[chunks objectAtIndex:0] isEqualToString:@"STRING"]) {
        [_delegate setString:[chunks objectAtIndex:1] value:[chunks objectAtIndex:2]];
    } else if ([[chunks objectAtIndex:0] isEqualToString:@"GET_MENU_ITEMS"]) {
        if ([[chunks objectAtIndex:1] isEqualToString:@"BEGIN"]) {
            [_delegate resetMenuItems];
        } else if ([[chunks objectAtIndex:1] isEqualToString:@"END"]) {
            // Don't do anything special, the askOnSocket call in FinderSync menuForMenuKind will return after this line
        }
    } else if ([[chunks objectAtIndex:0] isEqualToString:@"MENU_ITEM"]) {
        NSMutableDictionary *item = [[NSMutableDictionary alloc] init];
        [item setValue:[chunks objectAtIndex:1] forKey:@"command"]; // e.g. "COPY_PRIVATE_LINK"
        [item setValue:[chunks objectAtIndex:2] forKey:@"flags"]; // e.g. "d"
        [item setValue:[chunks objectAtIndex:3] forKey:@"text"]; // e.g. "Copy private link to clipboard"
        [_delegate addMenuItem:item];
    } else {
        NSLog(@"SyncState: Unknown command %@", [chunks objectAtIndex:0]);
    }
}

- (void)askOnSocket:(NSString *)path query:(NSString *)verb
{
    NSString *query = [NSString stringWithFormat:@"%@:%@\n", verb, path];

    @try {
        [_remoteEnd sendMessage:[query dataUsingEncoding:NSUTF8StringEncoding]];
    } @catch (NSException *e) {
        // Do nothing and wait for connectionDidDie
    }
}

- (void)askForIcon:(NSString *)path isDirectory:(BOOL)isDir
{
    NSString *verb = isDir ? @"RETRIEVE_FOLDER_STATUS" : @"RETRIEVE_FILE_STATUS";
    [self askOnSocket:path query:verb];
}

@end
