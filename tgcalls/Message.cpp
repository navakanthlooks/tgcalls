#include "Message.h"

#include "rtc_base/byte_buffer.h"
#include "api/jsep_ice_candidate.h"

namespace tgcalls {
namespace {

constexpr auto kMaxStringLength = 65536;

void Serialize(rtc::ByteBufferWriter &to, const std::string &from) {
	assert(from.size() < kMaxStringLength);

	to.WriteUInt32(uint32_t(from.size()));
	to.WriteString(from);
}

bool Deserialize(std::string &to, rtc::ByteBufferReader &from) {
	uint32_t length = 0;
	if (!from.ReadUInt32(&length)) {
		RTC_LOG(LS_ERROR) << "Could not read string length.";
		return false;
	} else if (length >= kMaxStringLength) {
		RTC_LOG(LS_ERROR) << "Invalid string length: " << length;
		return false;
	} else if (!from.ReadString(&to, length)) {
		RTC_LOG(LS_ERROR) << "Could not read string data.";
		return false;
	}
	return true;
}

void Serialize(rtc::ByteBufferWriter &to, const webrtc::SdpVideoFormat &from) {
	assert(from.parameters.size() < std::numeric_limits<uint8_t>::max());

	Serialize(to, from.name);
	to.WriteUInt8(uint8_t(from.parameters.size()));
	for (const auto &pair : from.parameters) {
		Serialize(to, pair.first);
		Serialize(to, pair.second);
	}
}

bool Deserialize(webrtc::SdpVideoFormat &to, rtc::ByteBufferReader &from) {
	if (!Deserialize(to.name, from)) {
		RTC_LOG(LS_ERROR) << "Could not read video format name.";
		return false;
	}
	auto count = uint8_t();
	if (!from.ReadUInt8(&count)) {
		RTC_LOG(LS_ERROR) << "Could not read video format parameters count.";
		return false;
	}
	for (uint32_t i = 0; i != count; ++i) {
		auto key = std::string();
		auto value = std::string();
		if (!Deserialize(key, from)) {
			RTC_LOG(LS_ERROR) << "Could not read video format parameter key.";
			return false;
		} else if (!Deserialize(value, from)) {
			RTC_LOG(LS_ERROR) << "Could not read video format parameter value.";
			return false;
		}
		to.parameters.emplace(key, value);
	}
	return true;
}

void Serialize(rtc::ByteBufferWriter &to, const cricket::Candidate &from) {
	webrtc::JsepIceCandidate iceCandidate{ std::string(), 0 };
	iceCandidate.SetCandidate(from);
	std::string serialized;
	const auto success = iceCandidate.ToString(&serialized);
	assert(success);
	Serialize(to, serialized);
}

bool Deserialize(cricket::Candidate &to, rtc::ByteBufferReader &from) {
	std::string candidate;
	if (!Deserialize(candidate, from)) {
		RTC_LOG(LS_ERROR) << "Could not read candidate string.";
		return false;
	}
	webrtc::JsepIceCandidate parseCandidate{ std::string(), 0 };
	if (!parseCandidate.Initialize(candidate, nullptr)) {
		RTC_LOG(LS_ERROR) << "Could not parse candidate: " << candidate;
		return false;
	}
	to = parseCandidate.candidate();
	return true;
}

void Serialize(rtc::ByteBufferWriter &to, const RequestVideoMessage &from, bool singleMessagePacket) {
}

bool Deserialize(RequestVideoMessage &to, rtc::ByteBufferReader &reader, bool singleMessagePacket) {
	return true;
}

void Serialize(rtc::ByteBufferWriter &to, const RemoteVideoIsActiveMessage &from, bool singleMessagePacket) {
	to.WriteUInt8(from.active ? 1 : 0);
}

bool Deserialize(RemoteVideoIsActiveMessage &to, rtc::ByteBufferReader &reader, bool singleMessagePacket) {
	uint8_t value = 0;
	if (!reader.ReadUInt8(&value)) {
		RTC_LOG(LS_ERROR) << "Could not read videoIsActive.";
		return false;
	}
	to.active = (value != 0);
	return true;
}

void Serialize(rtc::ByteBufferWriter &to, const CandidatesListMessage &from, bool singleMessagePacket) {
	assert(from.candidates.size() < std::numeric_limits<uint8_t>::max());

	to.WriteUInt8(uint8_t(from.candidates.size()));
	for (const auto &candidate : from.candidates) {
		Serialize(to, candidate);
	}
}

bool Deserialize(CandidatesListMessage &to, rtc::ByteBufferReader &reader, bool singleMessagePacket) {
	auto count = uint8_t();
	if (!reader.ReadUInt8(&count)) {
		RTC_LOG(LS_ERROR) << "Could not read candidates count.";
		return false;
	}
	for (uint32_t i = 0; i != count; ++i) {
		auto candidate = cricket::Candidate();
		if (!Deserialize(candidate, reader)) {
			RTC_LOG(LS_ERROR) << "Could not read candidate.";
			return false;
		}
		to.candidates.push_back(std::move(candidate));
	}
	return true;
}

void Serialize(rtc::ByteBufferWriter &to, const VideoFormatsMessage &from, bool singleMessagePacket) {
	assert(from.formats.size() < std::numeric_limits<uint8_t>::max());
	assert(from.encodersCount <= from.formats.size());

	to.WriteUInt8(uint8_t(from.formats.size()));
	for (const auto &format : from.formats) {
		Serialize(to, format);
	}
	to.WriteUInt8(uint8_t(from.encodersCount));
}

bool Deserialize(VideoFormatsMessage &to, rtc::ByteBufferReader &from, bool singleMessagePacket) {
	auto count = uint8_t();
	if (!from.ReadUInt8(&count)) {
		RTC_LOG(LS_ERROR) << "Could not read video formats count.";
		return false;
	}
	for (uint32_t i = 0; i != count; ++i) {
		auto format = webrtc::SdpVideoFormat(std::string());
		if (!Deserialize(format, from)) {
			RTC_LOG(LS_ERROR) << "Could not read video format.";
			return false;
		}
		to.formats.push_back(std::move(format));
	}
	auto encoders = uint8_t();
	if (!from.ReadUInt8(&encoders)) {
		RTC_LOG(LS_ERROR) << "Could not read encoders count.";
		return false;
	} else if (encoders > to.formats.size()) {
		RTC_LOG(LS_ERROR) << "Invalid encoders count: " << encoders << ", full formats count: " << to.formats.size();
		return false;
	}
	to.encodersCount = encoders;
	return true;
}

void Serialize(rtc::ByteBufferWriter &to, const rtc::CopyOnWriteBuffer &from, bool singleMessagePacket) {
	if (!singleMessagePacket) {
		assert(from.size() <= UINT16_MAX);
		to.WriteUInt16(from.size());
	}
	to.WriteBytes(reinterpret_cast<const char*>(from.cdata()), from.size());
}

bool Deserialize(rtc::CopyOnWriteBuffer &to, rtc::ByteBufferReader &from, bool singleMessagePacket) {
	auto length = uint16_t(from.Length());
	if (!singleMessagePacket) {
		if (!from.ReadUInt16(&length)) {
			RTC_LOG(LS_ERROR) << "Could not read buffer length.";
			return false;
		} else if (from.Length() < length) {
			RTC_LOG(LS_ERROR) << "Invalid buffer length: " << length << ", available: " << from.Length();
			return false;
		}
	}
	to.AppendData(from.Data(), length);
	from.Consume(length);
	return true;
}

void Serialize(rtc::ByteBufferWriter &to, const AudioDataMessage &from, bool singleMessagePacket) {
	Serialize(to, from.data, singleMessagePacket);
}

bool Deserialize(AudioDataMessage &to, rtc::ByteBufferReader &from, bool singleMessagePacket) {
	return Deserialize(to.data, from, singleMessagePacket);
}

void Serialize(rtc::ByteBufferWriter &to, const VideoDataMessage &from, bool singleMessagePacket) {
	Serialize(to, from.data, singleMessagePacket);
}

bool Deserialize(VideoDataMessage &to, rtc::ByteBufferReader &from, bool singleMessagePacket) {
	return Deserialize(to.data, from, singleMessagePacket);
}

void Serialize(rtc::ByteBufferWriter &to, const UnstructuredDataMessage &from, bool singleMessagePacket) {
    Serialize(to, from.data, singleMessagePacket);
}

bool Deserialize(UnstructuredDataMessage &to, rtc::ByteBufferReader &from, bool singleMessagePacket) {
    return Deserialize(to.data, from, singleMessagePacket);
}

template <typename T>
bool TryDeserialize(
		absl::optional<Message> &to,
		rtc::ByteBufferReader &reader,
		bool singleMessagePacket) {
	assert(reader.Length() != 0);

	constexpr auto id = T::kId;
	if (uint8_t(*reader.Data()) != id) {
		return false;
	}
	reader.Consume(1);
	auto parsed = T();
	if (!Deserialize(parsed, reader, singleMessagePacket)) {
		RTC_LOG(LS_ERROR) << "Could not read message with kId: " << id;
		return false;
	}
	to = Message{ parsed };
	return true;
}

template <typename ...Types>
struct TryDeserializeNext;

template <>
struct TryDeserializeNext<> {
	static bool Call(
			absl::optional<Message> &to,
			rtc::ByteBufferReader &reader,
			bool singleMessagePacket) {
		return false;
	}
};

template <typename T, typename ...Other>
struct TryDeserializeNext<T, Other...> {
	static bool Call(
			absl::optional<Message> &to,
			rtc::ByteBufferReader &reader,
			bool singleMessagePacket) {
		return TryDeserialize<T>(to, reader, singleMessagePacket)
			|| TryDeserializeNext<Other...>::Call(to, reader, singleMessagePacket);
	}
};

template <typename ...Types>
bool TryDeserializeRecursive(
		absl::optional<Message> &to,
		rtc::ByteBufferReader &reader,
		bool singleMessagePacket,
		absl::variant<Types...> *) {
	return TryDeserializeNext<Types...>::Call(to, reader, singleMessagePacket);
}

} // namespace


rtc::CopyOnWriteBuffer SerializeMessageWithSeq(
		const Message &message,
		uint32_t seq,
		bool singleMessagePacket) {
	rtc::ByteBufferWriter writer;
	writer.WriteUInt32(seq);
	absl::visit([&](const auto &data) {
		writer.WriteUInt8(std::decay_t<decltype(data)>::kId);
		Serialize(writer, data, singleMessagePacket);
	}, message.data);

	auto result = rtc::CopyOnWriteBuffer();
	result.AppendData(writer.Data(), writer.Length());

	return result;
}

absl::optional<Message> DeserializeMessage(
		rtc::ByteBufferReader &reader,
		bool singleMessagePacket) {
	if (!reader.Length()) {
		return absl::nullopt;
	}
	using Variant = decltype(std::declval<Message>().data);
	auto result = absl::make_optional<Message>();
	return TryDeserializeRecursive(result, reader, singleMessagePacket, (Variant*)nullptr)
		? result
		: absl::nullopt;
}

} // namespace tgcalls
