#pragma once

#include "Utils/Random.h"

#include <cstdint>
#include <functional>

namespace Daedalus 
{
	class UUID
	{
	public:
		UUID() : m_UUID(Random::UniformIntDistribution<uint64_t>()) {}
		UUID(uint64_t uuid) : m_UUID(uuid) {}
		UUID(const UUID&) = default;

		operator uint64_t() const { return m_UUID; }
	private:
		uint64_t m_UUID;
	};

}

namespace std 
{
	template<>
	struct hash<Daedalus::UUID>
	{
		std::size_t operator()(const Daedalus::UUID& uuid) const
		{
			return (uint64_t)uuid;
		}
	};

}