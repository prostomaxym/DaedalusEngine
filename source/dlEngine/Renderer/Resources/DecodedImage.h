#pragma once

#include "Macros.h"
#include <filesystem>
#include <memory>
#include <span>

namespace Daedalus
{
	// CPU-only image ownership; decoding never creates or accesses GPU resources.
	class DAEDALUS_API DecodedImage
	{
	public:
		DecodedImage() = default;
		~DecodedImage() = default;
		DecodedImage(const DecodedImage&) = delete;
		DecodedImage& operator=(const DecodedImage&) = delete;
		DecodedImage(DecodedImage&&) noexcept = default;
		DecodedImage& operator=(DecodedImage&&) noexcept = default;

		[[nodiscard]] bool Load(const std::filesystem::path& path, bool flip_vertically = true);
		[[nodiscard]] bool LoadMemory(std::span<const unsigned char> bytes, bool flip_vertically = true);
		unsigned char* GetPixels() const { return m_pixels.get(); }
		int GetWidth() const { return m_width; }
		int GetHeight() const { return m_height; }
		int GetChannels() const { return m_channels; }

	private:
		static void Free(unsigned char* pixels);
		void Reset();
		std::unique_ptr<unsigned char, void(*)(unsigned char*)> m_pixels{ nullptr, &Free };
		int m_width = 0;
		int m_height = 0;
		int m_channels = 0;
	};
}
