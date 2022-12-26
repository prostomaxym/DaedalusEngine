#pragma once

#include "BufferLayout.h"

#include <stdint.h>

namespace Daedalus {

class VertexBuffer
{
public:
	static std::shared_ptr<VertexBuffer> Create(uint32_t size);
	static std::shared_ptr<VertexBuffer> Create(const float* vertices, uint32_t size);

	virtual ~VertexBuffer() = default;

	virtual void Bind() const = 0;
	virtual void Unbind() const = 0;

	virtual const BufferLayout& GetLayout() const = 0;
	virtual void SetLayout(const BufferLayout& layout) = 0;

	virtual void SetData(const void* data, uint32_t size) = 0;
};

}
