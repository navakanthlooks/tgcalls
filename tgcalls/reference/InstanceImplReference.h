#ifndef TGCALLS_INSTANCE_IMPL_REFERENCE_H
#define TGCALLS_INSTANCE_IMPL_REFERENCE_H

#include "Instance.h"
#include "ThreadLocalObject.h"

namespace tgcalls {

class InstanceImplReferenceInternal;

class InstanceImplReference : public Instance {
public:
	explicit InstanceImplReference(Descriptor &&descriptor);
	~InstanceImplReference();

	void receiveSignalingData(const std::vector<uint8_t> &data) override;
	void setNetworkType(NetworkType networkType) override;
	void setMuteMicrophone(bool muteMicrophone) override;
	void requestVideo(std::shared_ptr<VideoCaptureInterface> videoCapture) override;
	void setIncomingVideoOutput(std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> sink) override;
	void setAudioOutputGainControlEnabled(bool enabled) override;
	void setEchoCancellationStrength(int strength) override;
	void setAudioInputDevice(std::string id) override;
	void setAudioOutputDevice(std::string id) override;
	void setInputVolume(float level) override;
	void setOutputVolume(float level) override;
	void setAudioOutputDuckingEnabled(bool enabled) override;

    static int GetConnectionMaxLayer();
    static std::string GetVersion();
	std::string getLastError() override;
	std::string getDebugInfo() override;
	int64_t getPreferredRelayId() override;
	TrafficStats getTrafficStats() override;
	PersistentState getPersistentState() override;
	FinalState stop() override;

private:
    std::unique_ptr<ThreadLocalObject<InstanceImplReferenceInternal>> internal_;
	std::function<void(State, VideoState)> onStateUpdated_;
	std::function<void(int)> onSignalBarsUpdated_;
};

} // namespace tgcalls

#endif
