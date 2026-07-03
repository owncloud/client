/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#import <Foundation/Foundation.h>


@protocol SyncClientProxyDelegate <NSObject>
- (void)setResultForPath:(NSString *)path result:(NSString *)result;
- (void)reFetchFileNameCacheForPath:(NSString *)path;
- (void)registerPath:(NSString *)path;
- (void)unregisterPath:(NSString *)path;
- (void)setString:(NSString *)key value:(NSString *)value;
- (void)resetMenuItems;
- (void)addMenuItem:(NSDictionary *)item;
- (void)connectionDidDie;
@end

@protocol ChannelProtocol <NSObject>
- (void)sendMessage:(NSData *)msg;
@end

@interface SyncClientProxy : NSObject <ChannelProtocol> {
    NSString *_serverName;
    NSDistantObject<ChannelProtocol> *_remoteEnd;
}

@property (weak) id<SyncClientProxyDelegate> delegate;

- (instancetype)initWithDelegate:(id)arg1 serverName:(NSString *)serverName;
- (void)start;
- (void)askOnSocket:(NSString *)path query:(NSString *)verb;
- (void)askForIcon:(NSString *)path isDirectory:(BOOL)isDir;
@end
