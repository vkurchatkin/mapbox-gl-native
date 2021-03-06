#import "MGLOfflinePack_Private.h"

#import "MGLOfflineStorage_Private.h"
#import "MGLOfflineRegion_Private.h"
#import "MGLTilePyramidOfflineRegion.h"

#include <mbgl/storage/default_file_source.hpp>
#include <mbgl/util/string.hpp>

/**
 Assert that the current offline pack is valid.
 
 This macro should be used at the beginning of any public-facing instance method
 of `MGLOfflinePack`. For private methods, an assertion is more appropriate.
 */
#define MGLAssertOfflinePackIsValid() \
    do { \
        if (_state == MGLOfflinePackStateInvalid) { \
            [NSException raise:@"Invalid offline pack" \
                        format: \
             @"-[MGLOfflineStorage removePack:withCompletionHandler:] has been called " \
             @"on this instance of MGLOfflinePack, rendering it invalid. It is an " \
             @"error to send any message to this pack."]; \
        } \
    } while (NO);

class MBGLOfflineRegionObserver : public mbgl::OfflineRegionObserver {
public:
    MBGLOfflineRegionObserver(MGLOfflinePack *pack_) : pack(pack_) {}
    
    void statusChanged(mbgl::OfflineRegionStatus status) override;
    void responseError(mbgl::Response::Error error) override;
    void mapboxTileCountLimitExceeded(uint64_t limit) override;
    
private:
    __weak MGLOfflinePack *pack = nullptr;
};

@interface MGLOfflinePack ()

@property (nonatomic, nullable, readwrite) mbgl::OfflineRegion *mbglOfflineRegion;
@property (nonatomic, readwrite) MGLOfflinePackState state;
@property (nonatomic, readwrite) MGLOfflinePackProgress progress;

@end

@implementation MGLOfflinePack

- (instancetype)init {
    [NSException raise:@"Method unavailable"
                format:
     @"-[MGLOfflinePack init] is unavailable. "
     @"Use +[MGLOfflineStorage addPackForRegion:withContext:completionHandler:] instead."];
    return nil;
}

- (instancetype)initWithMBGLRegion:(mbgl::OfflineRegion *)region {
    if (self = [super init]) {
        _mbglOfflineRegion = region;
        _state = MGLOfflinePackStateUnknown;
        
        mbgl::DefaultFileSource *mbglFileSource = [[MGLOfflineStorage sharedOfflineStorage] mbglFileSource];
        mbglFileSource->setOfflineRegionObserver(*_mbglOfflineRegion, std::make_unique<MBGLOfflineRegionObserver>(self));
    }
    return self;
}

- (void)dealloc {
    if (_mbglOfflineRegion && _state != MGLOfflinePackStateInvalid) {
        mbgl::DefaultFileSource *mbglFileSource = [[MGLOfflineStorage sharedOfflineStorage] mbglFileSource];
        mbglFileSource->setOfflineRegionObserver(*_mbglOfflineRegion, nullptr);
    }
}

- (id <MGLOfflineRegion>)region {
    MGLAssertOfflinePackIsValid();
    
    const mbgl::OfflineRegionDefinition &regionDefinition = _mbglOfflineRegion->getDefinition();
    NSAssert([MGLTilePyramidOfflineRegion conformsToProtocol:@protocol(MGLOfflineRegion_Private)], @"MGLTilePyramidOfflineRegion should conform to MGLOfflineRegion_Private.");
    return [(id <MGLOfflineRegion_Private>)[MGLTilePyramidOfflineRegion alloc] initWithOfflineRegionDefinition:regionDefinition];
}

- (NSData *)context {
    MGLAssertOfflinePackIsValid();
    
    const mbgl::OfflineRegionMetadata &metadata = _mbglOfflineRegion->getMetadata();
    return [NSData dataWithBytes:&metadata[0] length:metadata.size()];
}

- (void)resume {
    MGLAssertOfflinePackIsValid();
    
    mbgl::DefaultFileSource *mbglFileSource = [[MGLOfflineStorage sharedOfflineStorage] mbglFileSource];
    mbglFileSource->setOfflineRegionDownloadState(*_mbglOfflineRegion, mbgl::OfflineRegionDownloadState::Active);
}

- (void)suspend {
    MGLAssertOfflinePackIsValid();
    
    mbgl::DefaultFileSource *mbglFileSource = [[MGLOfflineStorage sharedOfflineStorage] mbglFileSource];
    mbglFileSource->setOfflineRegionDownloadState(*_mbglOfflineRegion, mbgl::OfflineRegionDownloadState::Inactive);
}

- (void)invalidate {
    NSAssert(_state != MGLOfflinePackStateInvalid, @"Cannot invalidate an already invalid offline pack.");
    
    self.state = MGLOfflinePackStateInvalid;
    self.mbglOfflineRegion = nil;
}

- (void)setState:(MGLOfflinePackState)state {
    if (!self.mbglOfflineRegion) {
        // A progress update has arrived after the call to
        // -[MGLOfflineStorage removePack:withCompletionHandler:] but before the
        // removal is complete and the completion handler is called.
        NSAssert(_state == MGLOfflinePackStateInvalid, @"A valid MGLOfflinePack has no mbgl::OfflineRegion.");
        return;
    }
    
    NSAssert(_state != MGLOfflinePackStateInvalid, @"Cannot change the state of an invalid offline pack.");
    
    _state = state;
}

- (void)requestProgress {
    MGLAssertOfflinePackIsValid();
    
    mbgl::DefaultFileSource *mbglFileSource = [[MGLOfflineStorage sharedOfflineStorage] mbglFileSource];
    
    __weak MGLOfflinePack *weakSelf = self;
    mbglFileSource->getOfflineRegionStatus(*_mbglOfflineRegion, [&, weakSelf](__unused std::exception_ptr exception, mbgl::optional<mbgl::OfflineRegionStatus> status) {
        if (status) {
            mbgl::OfflineRegionStatus checkedStatus = *status;
            dispatch_async(dispatch_get_main_queue(), ^{
                MGLOfflinePack *strongSelf = weakSelf;
                [strongSelf offlineRegionStatusDidChange:checkedStatus];
            });
        }
    });
}

- (void)offlineRegionStatusDidChange:(mbgl::OfflineRegionStatus)status {
    NSAssert(_state != MGLOfflinePackStateInvalid, @"Cannot change update progress of an invalid offline pack.");
    
    switch (status.downloadState) {
        case mbgl::OfflineRegionDownloadState::Inactive:
            self.state = status.complete() ? MGLOfflinePackStateComplete : MGLOfflinePackStateInactive;
            break;
            
        case mbgl::OfflineRegionDownloadState::Active:
            self.state = MGLOfflinePackStateActive;
            break;
    }
    
    MGLOfflinePackProgress progress;
    progress.countOfResourcesCompleted = status.completedResourceCount;
    progress.countOfBytesCompleted = status.completedResourceSize;
    progress.countOfResourcesExpected = status.requiredResourceCount;
    progress.maximumResourcesExpected = status.requiredResourceCountIsPrecise ? status.requiredResourceCount : UINT64_MAX;
    self.progress = progress;
    
    if ([self.delegate respondsToSelector:@selector(offlinePack:progressDidChange:)]) {
        [self.delegate offlinePack:self progressDidChange:progress];
    }
}

NSError *MGLErrorFromResponseError(mbgl::Response::Error error) {
    NSInteger errorCode = MGLErrorCodeUnknown;
    switch (error.reason) {
        case mbgl::Response::Error::Reason::NotFound:
            errorCode = MGLErrorCodeNotFound;
            break;
            
        case mbgl::Response::Error::Reason::Server:
            errorCode = MGLErrorCodeBadServerResponse;
            break;
            
        case mbgl::Response::Error::Reason::Connection:
            errorCode = MGLErrorCodeConnectionFailed;
            break;
            
        default:
            break;
    }
    return [NSError errorWithDomain:MGLErrorDomain code:errorCode userInfo:@{
        NSLocalizedFailureReasonErrorKey: @(error.message.c_str())
    }];
}

void MBGLOfflineRegionObserver::statusChanged(mbgl::OfflineRegionStatus status) {
    dispatch_async(dispatch_get_main_queue(), ^{
        [pack offlineRegionStatusDidChange:status];
    });
}

void MBGLOfflineRegionObserver::responseError(mbgl::Response::Error error) {
    dispatch_async(dispatch_get_main_queue(), ^{
        if ([pack.delegate respondsToSelector:@selector(offlinePack:didReceiveError:)]) {
            [pack.delegate offlinePack:pack didReceiveError:MGLErrorFromResponseError(error)];
        }
    });
}

void MBGLOfflineRegionObserver::mapboxTileCountLimitExceeded(uint64_t limit) {
    dispatch_async(dispatch_get_main_queue(), ^{
        if ([pack.delegate respondsToSelector:@selector(offlinePack:didReceiveMaximumAllowedMapboxTiles:)]) {
            [pack.delegate offlinePack:pack didReceiveMaximumAllowedMapboxTiles:limit];
        }
    });
}

@end
