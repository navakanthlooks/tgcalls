#include "DarwinInterface.h"

#include "VideoCapturerInterfaceImpl.h"
#include "sdk/objc/native/src/objc_video_track_source.h"

#include "media/base/media_constants.h"
#include "TGRTCDefaultVideoEncoderFactory.h"
#include "TGRTCDefaultVideoDecoderFactory.h"
#include "sdk/objc/native/api/video_encoder_factory.h"
#include "sdk/objc/native/api/video_decoder_factory.h"
#include "api/video_track_source_proxy.h"

#ifdef WEBRTC_IOS
#include "sdk/objc/components/audio/RTCAudioSession.h"
#endif // WEBRTC_IOS

#import <AVFoundation/AVFoundation.h>

namespace tgcalls {

void DarwinInterface::configurePlatformAudio() {
    [RTCAudioSession sharedInstance].useManualAudio = true;
    [RTCAudioSession sharedInstance].isAudioEnabled = true;
}

std::unique_ptr<webrtc::VideoEncoderFactory> DarwinInterface::makeVideoEncoderFactory() {
    return webrtc::ObjCToNativeVideoEncoderFactory([[TGRTCDefaultVideoEncoderFactory alloc] init]);
}

std::unique_ptr<webrtc::VideoDecoderFactory> DarwinInterface::makeVideoDecoderFactory() {
    return webrtc::ObjCToNativeVideoDecoderFactory([[TGRTCDefaultVideoDecoderFactory alloc] init]);
}

bool DarwinInterface::supportsEncoding(const std::string &codecName) {
	if (codecName == cricket::kH265CodecName) {
#ifdef WEBRTC_IOS
		if (@available(iOS 11.0, *)) {
			return [[AVAssetExportSession allExportPresets] containsObject:AVAssetExportPresetHEVCHighestQuality];
		}
#elif defined WEBRTC_MAC // WEBRTC_IOS
		if (@available(macOS 10.13, *)) {
			return [[AVAssetExportSession allExportPresets] containsObject:AVAssetExportPresetHEVCHighestQuality];
		}
#endif // WEBRTC_IOS || WEBRTC_MAC
    } else if (codecName == cricket::kH264CodecName) {
        return true;
    } else if (codecName == cricket::kVp8CodecName) {
        return true;
    } else if (codecName == cricket::kVp9CodecName) {
        #ifndef WEBRTC_IOS
        return true;
        #else
        return false;
        #endif
    }
    return false;
}

rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> DarwinInterface::makeVideoSource(rtc::Thread *signalingThread, rtc::Thread *workerThread) {
    rtc::scoped_refptr<webrtc::ObjCVideoTrackSource> objCVideoTrackSource(new rtc::RefCountedObject<webrtc::ObjCVideoTrackSource>());
    return webrtc::VideoTrackSourceProxy::Create(signalingThread, workerThread, objCVideoTrackSource);
}

std::unique_ptr<VideoCapturerInterface> DarwinInterface::makeVideoCapturer(rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> source, bool useFrontCamera, std::function<void(bool)> isActiveUpdated) {
    return std::make_unique<VideoCapturerInterfaceImpl>(source, useFrontCamera, isActiveUpdated);
}

std::unique_ptr<PlatformInterface> CreatePlatformInterface() {
	return std::make_unique<DarwinInterface>();
}

} // namespace tgcalls
