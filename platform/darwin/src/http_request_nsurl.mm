#include <mbgl/storage/http_context_base.hpp>
#include <mbgl/storage/http_request_base.hpp>
#include <mbgl/storage/resource.hpp>
#include <mbgl/storage/response.hpp>

#include <mbgl/util/async_task.hpp>
#include <mbgl/util/run_loop.hpp>

#include "version.hpp"

#import <Foundation/Foundation.h>

#include <map>
#include <cassert>
#include <mutex>

@interface MBGLBundleCanary : NSObject
@end

@implementation MBGLBundleCanary
@end

namespace mbgl {

class HTTPNSURLContext;

class HTTPNSURLRequest : public HTTPRequestBase {
public:
    HTTPNSURLRequest(HTTPNSURLContext*, Resource, Callback);
    ~HTTPNSURLRequest();

    void cancel() final;

private:
    static std::unique_ptr<Response> handleResult(NSData *data, NSURLResponse *res, NSError *error, Resource);
    void handleResponse();

    HTTPNSURLContext *context = nullptr;
    std::shared_ptr<std::pair<bool, std::mutex>> cancelled;
    NSURLSessionDataTask *task = nullptr;
    std::unique_ptr<Response> response;
    util::AsyncTask async;
};

// -------------------------------------------------------------------------------------------------

class HTTPNSURLContext : public HTTPContextBase {
public:
    HTTPNSURLContext();
    ~HTTPNSURLContext();

    HTTPRequestBase* createRequest(const Resource&, HTTPRequestBase::Callback) final;

    NSURLSession *session = nil;
    NSString *userAgent = nil;
    NSInteger accountType = 0;

private:
    NSString *getUserAgent();
    NSBundle *getSDKBundle();
};

HTTPNSURLContext::HTTPNSURLContext() {
    @autoreleasepool {
        NSURLSessionConfiguration* sessionConfig =
            [NSURLSessionConfiguration defaultSessionConfiguration];
        sessionConfig.timeoutIntervalForResource = 30;
        sessionConfig.HTTPMaximumConnectionsPerHost = 8;
        sessionConfig.requestCachePolicy = NSURLRequestReloadIgnoringLocalCacheData;
        sessionConfig.URLCache = nil;

        session = [NSURLSession sessionWithConfiguration:sessionConfig];
        [session retain];

        userAgent = HTTPNSURLContext::getUserAgent();

        accountType = [[NSUserDefaults standardUserDefaults] integerForKey:@"MGLMapboxAccountType"];
    }
}

HTTPNSURLContext::~HTTPNSURLContext() {
    [session release];
    session = nullptr;

    [userAgent release];
    userAgent = nullptr;
}

HTTPRequestBase* HTTPNSURLContext::createRequest(const Resource& resource, HTTPRequestBase::Callback callback) {
    return new HTTPNSURLRequest(this, resource, callback);
}

NSString *HTTPNSURLContext::getUserAgent() {
    NSMutableArray *userAgentComponents = [NSMutableArray array];
    
    NSBundle *appBundle = [NSBundle mainBundle];
    if (appBundle) {
        [userAgentComponents addObject:[NSString stringWithFormat:@"%@/%@",
                                        appBundle.infoDictionary[@"CFBundleName"],
                                        appBundle.infoDictionary[@"CFBundleShortVersionString"]]];
    } else {
        [userAgentComponents addObject:[NSProcessInfo processInfo].processName];
    }
    
    NSBundle *sdkBundle = HTTPNSURLContext::getSDKBundle();
    if (sdkBundle) {
        NSString *versionString = sdkBundle.infoDictionary[@"MGLSemanticVersionString"];
        if (!versionString) {
            versionString = sdkBundle.infoDictionary[@"CFBundleShortVersionString"];
        }
        if (versionString) {
            [userAgentComponents addObject:[NSString stringWithFormat:@"%@/%@",
                                            sdkBundle.infoDictionary[@"CFBundleName"], versionString]];
        }
    }
    
    // Avoid %s here because it inserts hidden bidirectional markers on OS X when the system
    // language is set to a right-to-left language.
    [userAgentComponents addObject:[NSString stringWithFormat:@"MapboxGL/%@ (%@)",
                                    CFSTR(MBGL_VERSION_STRING), CFSTR(MBGL_VERSION_REV)]];
    
    NSString *systemName = @"Darwin";
#if TARGET_OS_IPHONE
    systemName = @"iOS";
#elif TARGET_OS_MAC
    systemName = @"OS X";
#elif TARGET_OS_WATCH
    systemName = @"watchOS";
#elif TARGET_OS_TV
    systemName = @"tvOS";
#endif
    NSString *systemVersion = nil;
    if ([NSProcessInfo instancesRespondToSelector:@selector(operatingSystemVersion)]) {
        NSOperatingSystemVersion osVersion = [NSProcessInfo processInfo].operatingSystemVersion;
        systemVersion = [NSString stringWithFormat:@"%ld.%ld.%ld",
                         (long)osVersion.majorVersion, (long)osVersion.minorVersion, (long)osVersion.patchVersion];
    }
    if (systemVersion) {
        [userAgentComponents addObject:[NSString stringWithFormat:@"%@/%@", systemName, systemVersion]];
    }
    
    NSString *cpu = nil;
#if TARGET_CPU_X86
    cpu = @"x86";
#elif TARGET_CPU_X86_64
    cpu = @"x86_64";
#elif TARGET_CPU_ARM
    cpu = @"arm";
#elif TARGET_CPU_ARM64
    cpu = @"arm64";
#endif
    if (cpu) {
        [userAgentComponents addObject:[NSString stringWithFormat:@"(%@)", cpu]];
    }
    
    return [userAgentComponents componentsJoinedByString:@" "];
}

NSBundle *HTTPNSURLContext::getSDKBundle() {
    NSBundle *bundle = [NSBundle bundleForClass:[MBGLBundleCanary class]];
    if (bundle && ![bundle.infoDictionary[@"CFBundlePackageType"] isEqualToString:@"FMWK"]) {
        // For static frameworks, the class is contained in the application bundle rather than the
        // framework bundle.
        bundle = [NSBundle bundleWithPath:[bundle.privateFrameworksPath
                                           stringByAppendingPathComponent:@"Mapbox.framework"]];
    }
    return bundle;
}

// -------------------------------------------------------------------------------------------------

HTTPNSURLRequest::HTTPNSURLRequest(HTTPNSURLContext* context_,
                                   Resource resource_,
                                   Callback callback_)
    : HTTPRequestBase(resource_, callback_),
      context(context_),
      async([this] { handleResponse(); }) {

    // Ensure that a stack-allocated std::shared_ptr gets copied into the Objective-C
    // block used as the completion handler below. Objective-C will implicitly copy captured
    // stack variables. For member variable access it will implicitly copy the this pointer.
    // That wouldn't work here because we need the block to have its own shared_ptr.
    auto cancelled_ = cancelled = std::make_shared<std::pair<bool, std::mutex>>();
    cancelled->first = false;

    @autoreleasepool {
        NSURL* url = [NSURL URLWithString:@(resource.url.c_str())];
        if (context->accountType == 0 &&
            ([url.host isEqualToString:@"mapbox.com"] || [url.host hasSuffix:@".mapbox.com"])) {
            NSString* absoluteString = [url.absoluteString
                stringByAppendingFormat:(url.query ? @"&%@" : @"?%@"), @"events=true"];
            url = [NSURL URLWithString:absoluteString];
        }

        NSMutableURLRequest* req = [NSMutableURLRequest requestWithURL:url];
        if (resource.priorEtag) {
            [req addValue:@(resource.priorEtag->c_str())
                 forHTTPHeaderField:@"If-None-Match"];
        } else if (resource.priorModified) {
            [req addValue:@(util::rfc1123(*resource.priorModified).c_str())
                 forHTTPHeaderField:@"If-Modified-Since"];
        }

        [req addValue:context->userAgent forHTTPHeaderField:@"User-Agent"];

        task = [context->session
            dataTaskWithRequest:req
              completionHandler:^(NSData* data, NSURLResponse* res, NSError* error) {
                    std::unique_ptr<Response> response_ = HTTPNSURLRequest::handleResult(data, res, error, resource_);
                    std::lock_guard<std::mutex> lock(cancelled_->second);
                    if (!cancelled_->first) {
                        response = std::move(response_);
                        async.send();
                    }
              }];
        [task retain];
        [task resume];
    }
}

HTTPNSURLRequest::~HTTPNSURLRequest() {
    assert(!task);
}

void HTTPNSURLRequest::handleResponse() {
    if (task) {
        [task release];
        task = nullptr;
    }

    assert(response);
    notify(*response);

    delete this;
}

void HTTPNSURLRequest::cancel() {
    if (task) {
        [task cancel];
        [task release];
        task = nullptr;
    }

    std::lock_guard<std::mutex> lock(cancelled->second);
    cancelled->first = true;

    delete this;
}

std::unique_ptr<Response> HTTPNSURLRequest::handleResult(NSData *data, NSURLResponse *res, NSError *error, Resource resource) {
    std::unique_ptr<Response> response = std::make_unique<Response>();
    using Error = Response::Error;

    if (error) {
        if ([error code] == NSURLErrorCancelled) {
            response.reset();

        } else {
            if (data) {
                response->data =
                    std::make_shared<std::string>((const char*)[data bytes], [data length]);
            }

            switch ([error code]) {
            case NSURLErrorBadServerResponse: // 5xx errors
                response->error = std::make_unique<Error>(
                    Error::Reason::Server, [[error localizedDescription] UTF8String]);
                break;

            case NSURLErrorNetworkConnectionLost:
            case NSURLErrorCannotFindHost:
            case NSURLErrorCannotConnectToHost:
            case NSURLErrorDNSLookupFailed:
            case NSURLErrorNotConnectedToInternet:
            case NSURLErrorInternationalRoamingOff:
            case NSURLErrorCallIsActive:
            case NSURLErrorDataNotAllowed:
            case NSURLErrorTimedOut:
                response->error = std::make_unique<Error>(
                    Error::Reason::Connection, [[error localizedDescription] UTF8String]);
                break;

            default:
                response->error = std::make_unique<Error>(
                    Error::Reason::Other, [[error localizedDescription] UTF8String]);
                break;
            }
        }
    } else if ([res isKindOfClass:[NSHTTPURLResponse class]]) {
        const long responseCode = [(NSHTTPURLResponse *)res statusCode];

        NSDictionary *headers = [(NSHTTPURLResponse *)res allHeaderFields];
        NSString *cache_control = [headers objectForKey:@"Cache-Control"];
        if (cache_control) {
            response->expires = parseCacheControl([cache_control UTF8String]);
        }

        NSString *expires = [headers objectForKey:@"Expires"];
        if (expires) {
            response->expires = util::parseTimePoint([expires UTF8String]);
        }

        NSString *last_modified = [headers objectForKey:@"Last-Modified"];
        if (last_modified) {
            response->modified = util::parseTimePoint([last_modified UTF8String]);
        }

        NSString *etag = [headers objectForKey:@"ETag"];
        if (etag) {
            response->etag = std::string([etag UTF8String]);
        }

        if (responseCode == 200) {
            response->data = std::make_shared<std::string>((const char *)[data bytes], [data length]);
        } else if (responseCode == 204 || (responseCode == 404 && resource.kind == Resource::Kind::Tile)) {
            response->noContent = true;
        } else if (responseCode == 304) {
            response->notModified = true;
        } else if (responseCode == 404) {
            response->error =
                std::make_unique<Error>(Error::Reason::NotFound, "HTTP status code 404");
        } else if (responseCode >= 500 && responseCode < 600) {
            response->error =
                std::make_unique<Error>(Error::Reason::Server, std::string{ "HTTP status code " } +
                                                                   std::to_string(responseCode));
        } else {
            response->error =
                std::make_unique<Error>(Error::Reason::Other, std::string{ "HTTP status code " } +
                                                                  std::to_string(responseCode));
        }
    } else {
        // This should never happen.
        response->error = std::make_unique<Error>(Error::Reason::Other,
                                                  "Response class is not NSHTTPURLResponse");
    }

    return response;
}

std::unique_ptr<HTTPContextBase> HTTPContextBase::createContext() {
    return std::make_unique<HTTPNSURLContext>();
}

uint32_t HTTPContextBase::maximumConcurrentRequests() {
    return 20;
}

}
