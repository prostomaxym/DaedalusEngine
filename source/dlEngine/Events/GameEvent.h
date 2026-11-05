#pragma once

#include "Event.h"
#include <cstdint>
#include <utility>

namespace Daedalus
{
	class DAEDALUS_API GameEvent final : public Event
	{
	public:
		GameEvent(std::string topic, std::string payload = {}, uint64_t sender = 0)
			: m_topic(std::move(topic)), m_payload(std::move(payload)), m_sender(sender) {}
		const std::string& GetTopic() const { return m_topic; }
		const std::string& GetPayload() const { return m_payload; }
		uint64_t GetSender() const { return m_sender; }
		EVENT_CLASS_TYPE(GameMessage)
		EVENT_CLASS_CATEGORY(GameEventCategory)

	private:
		std::string m_topic;
		std::string m_payload;
		uint64_t m_sender;
	};
}
