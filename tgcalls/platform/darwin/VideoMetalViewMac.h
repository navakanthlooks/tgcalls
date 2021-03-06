#ifndef TGCALLS_VIDEO_METAL_VIEW_MAC_H
#define TGCALLS_VIDEO_METAL_VIEW_MAC_H
#ifndef WEBRTC_IOS
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#import "api/media_stream_interface.h"

#include <memory>

@class RTCVideoFrame;

@interface VideoMetalView : NSView

+ (bool)isSupported;

@property(nonatomic) CALayerContentsGravity _Nullable videoContentMode;
@property(nonatomic, getter=isEnabled) BOOL enabled;
@property(nonatomic, nullable) NSValue* rotationOverride;

- (void)setSize:(CGSize)size;
- (void)renderFrame:(nullable RTCVideoFrame *)frame;

- (std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>>)getSink;
- (void)setOnFirstFrameReceived:(void (^ _Nullable)())onFirstFrameReceived;


@end

#endif // WEBRTC_MAC
#endif
