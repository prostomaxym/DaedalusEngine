#pragma once

#include <stdint.h>

namespace Daedalus {

class IndexBuffer
{
public:
	static std::shared_ptr<IndexBuffer> Create(const uint32_t* indices, uint32_t count);

	virtual ~IndexBuffer() = default;

	virtual void Bind() const = 0;
	virtual void Unbind() const = 0;

	virtual uint32_t GetCount() const = 0;
};

}
