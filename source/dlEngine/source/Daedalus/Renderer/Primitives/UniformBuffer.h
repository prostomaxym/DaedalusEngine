#pragma once

#include <memory>

namespace Daedalus {

	class UniformBuffer
	{
	public:
		enum class Type
		{
			Static,
			Dynamic
		};

	public:
		virtual ~UniformBuffer() = default;
		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

		static std::shared_ptr<UniformBuffer> Create(uint32_t size, uint32_t binding, Type type);
	};

}
