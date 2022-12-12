#pragma once

#include "IndexBuffer.h"
#include "VertexBuffer.h"

#include <memory>
#include <vector>

namespace Daedalus {

	class VertexArray
	{
	public:
		virtual ~VertexArray() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		static std::unique_ptr<VertexArray> Create();
	};

}
