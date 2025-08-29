#include <QString>

#import <Foundation/NSString.h>
#import <UserNotifications/UserNotifications.h>
#import <dispatch/dispatch.h>

@interface OurDelegate : NSObject <UNUserNotificationCenterDelegate>

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler;
@end

@implementation OurDelegate

// Always show, even if app is active at the moment.
- (void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler
{
    Q_UNUSED(center)
    Q_UNUSED(notification)
    completionHandler(UNNotificationPresentationOptionList | UNNotificationPresentationOptionBanner);
}

@end

namespace OCC {

void *createOsXNotificationCenterDelegate()
{
    auto delegate = [[OurDelegate alloc] init];
    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    [center requestAuthorizationWithOptions:(UNAuthorizationOptionBadge | UNAuthorizationOptionSound | UNAuthorizationOptionAlert)
                          completionHandler:^(BOOL granted, NSError *_Nullable error) {
                              Q_UNUSED(granted)
                              Q_UNUSED(error)
                          }];
    [center setDelegate:delegate];
    return delegate;
}

void releaseOsXNotificationCenterDelegate(void *delegate)
{
    [static_cast<OurDelegate *>(delegate) release];
}

void sendOsXUserNotification(const QString &title, const QString &message)
{
    @autoreleasepool {
        UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
        content.title = [NSString stringWithUTF8String:title.toUtf8().data()];
        content.body = [NSString stringWithUTF8String:message.toUtf8().data()];

        UNTimeIntervalNotificationTrigger *trigger = [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:1 repeats:NO];
        UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:@"de.owncloud.client.notification" content:content trigger:trigger];


        [[UNUserNotificationCenter currentNotificationCenter] addNotificationRequest:request withCompletionHandler:nil];
    }
}

} // namespace OCC
