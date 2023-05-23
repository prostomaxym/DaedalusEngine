#pragma once

#include <random>
#include <optional>

#include "Daedalus/Core/Core.h"

namespace Daedalus 
{
	class DAEDALUS_API Random
	{
	public:
		template<typename T>
		static T UniformDistribution(std::optional<T> from = std::nullopt, std::optional<T> to = std::nullopt)
		{
			if (from.has_value() && to.has_value())
			{
				std::uniform_int_distribution<T> distribution(from.value(), to.value());
				return distribution(s_engine);
			}
			else
			{
				std::uniform_int_distribution<T> distribution;
				return distribution(s_engine);
			}
		}

	private:
		Random() = delete;
		Random(const Random&) = delete;
		Random(Random&&) = delete;
		Random& operator=(const Random&) = delete;
		Random& operator=(Random&&) = delete;
		~Random() = delete;

		static std::random_device s_random_device;
		static std::mt19937_64 s_engine;
	};
}
