#import <Foundation/Foundation.h>

#import "MGLTypes.h"

NS_ASSUME_NONNULL_BEGIN

@class MGLOfflinePack;
@protocol MGLOfflineRegion;

extern NSString * const MGLOfflinePackProgressChangedNotification;
extern NSString * const MGLOfflinePackErrorNotification;
extern NSString * const MGLOfflinePackMaximumMapboxTilesReachedNotification;

extern NSString * const MGLOfflinePackErrorUserInfoKey;
extern NSString * const MGLOfflinePackMaximumCountUserInfoKey;

/**
 A block to be called once an offline pack has been completely created and
 added.
 
 @param pack Contains a pointer to the newly added pack, or `nil` if there was
    an error creating or adding the pack.
 @param error Contains a pointer to an error object (if any) indicating why the
    pack could not be created or added. For a list of possible error codes, see
    `MGLErrorCode`.
 */
typedef void (^MGLOfflinePackAdditionCompletionHandler)(MGLOfflinePack * _Nullable pack, NSError * _Nullable error);

/**
 A block to be called once an offline pack has been completely invalidated and
 removed.
 
 @param error Contains a pointer to an error object (if any) indicating why the
    pack could not be invalidated or removed.
 */
typedef void (^MGLOfflinePackRemovalCompletionHandler)(NSError * _Nullable error);

/**
 MGLOfflineStorage implements a singleton (shared object) that manages offline
 packs. All of this class’s instance methods are asynchronous, reflecting the
 fact that offline resources are stored in a database.
 */
@interface MGLOfflineStorage : NSObject

/**
 Returns the shared offline storage object.
 */
+ (instancetype)sharedOfflineStorage;

/**
 An array of all known offline packs.
 
 This property is set to `nil`, indicating that the receiver does not yet know
 the existing packs, for an undefined amount of time starting from the moment
 the shared offline storage object is initialized until the packs are fetched
 from the database. After that point, this property is always non-nil, but it
 may be empty to indicate that no packs are present.
 */
@property (nonatomic, copy, readonly, nullable) NS_ARRAY_OF(MGLOfflinePack *) *packs;

/**
 Creates and registers an offline pack that downloads the resources needed to
 use the given region offline.
 
 The resulting pack starts out with a state of `MGLOfflinePackStateInactive`. To
 begin downloading resources, call `-[MGLOfflinePack resume]`. To monitor
 download progress, set the pack’s `delegate` property to an object that
 conforms to the `MGLOfflinePackDelegate` protocol.
 
 @param region A region to download.
 @param context Arbitrary data to store alongside the downloaded resources.
 @param completion The completion handler to call once the pack has been added.
    This handler is executed asynchronously on the main queue.
 */
- (void)addPackForRegion:(id <MGLOfflineRegion>)region withContext:(NSData *)context completionHandler:(nullable MGLOfflinePackAdditionCompletionHandler)completion;

/**
 Unregisters the given offline pack and frees any resources that are no longer
 required by any remaining packs.
 
 As soon as this method is called on a pack, the pack becomes invalid; any
 attempt to send it a message will result in an exception being thrown. If an
 error occurs and the pack cannot be removed, do not attempt to reuse the pack
 object. Instead, use the `-getPacksWithCompletionHandler:` method to obtain a
 valid pointer to the pack object.
 
 @param pack The offline pack to remove.
 @param completion The completion handler to call once the pack has been
    removed. This handler is executed asynchronously on the main queue.
 */
- (void)removePack:(MGLOfflinePack *)pack withCompletionHandler:(nullable MGLOfflinePackRemovalCompletionHandler)completion;

/**
 Sets the maximum number of Mapbox-hosted tiles that may be downloaded and
 stored on the current device.
 
 Once this limit is reached,
 `-[MGLOfflinePackDelegate offlinePack:didReceiveMaximumAllowedMapboxTiles:]` is
 called on every delegate of `MGLOfflinePack` until already downloaded tiles are
 removed by calling the `-removePack:withCompletionHandler:` method.
 
 @note The [Mapbox Terms of Service](https://www.mapbox.com/tos/) prohibits
    changing or bypassing this limit without permission from Mapbox. Contact
    your Mapbox sales representative to have the limit raised.
 */
- (void)setMaximumAllowedMapboxTiles:(uint64_t)maximumCount;

@end

NS_ASSUME_NONNULL_END
