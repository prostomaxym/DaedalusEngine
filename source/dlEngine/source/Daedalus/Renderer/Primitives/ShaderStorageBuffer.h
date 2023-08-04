#pragma once

#include <memory>

namespace Daedalus {

	class ShaderStorageBuffer
	{
	public:
		enum class Type
		{
			Static,
			Dynamic
		};

	public:
		virtual ~ShaderStorageBuffer() = default;
		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) const = 0;
		virtual void Bind(uint32_t binding) const = 0;

		static std::shared_ptr<ShaderStorageBuffer> Create(uint32_t size, uint32_t binding, Type type);
	};

}
