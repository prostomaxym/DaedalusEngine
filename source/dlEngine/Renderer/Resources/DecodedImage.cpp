#include "dlpch.h"
#include "DecodedImage.h"

#include <limits>
#include <stb_image.h>
#include <cstdlib>
#include <cstring>

using namespace Daedalus;

void DecodedImage::Free(unsigned char* pixels) { stbi_image_free(pixels); }

void DecodedImage::Reset()
{
	m_pixels.reset();
	m_width = m_height = m_channels = 0;
}

bool DecodedImage::Load(const std::filesystem::path& path, bool flip_vertically)
{
	Reset();
	stbi_set_flip_vertically_on_load_thread(flip_vertically ? 1 : 0);
	m_pixels.reset(stbi_load(path.string().c_str(), &m_width, &m_height, &m_channels, 0));
	if (!m_pixels) Reset();
	return static_cast<bool>(m_pixels);
}

bool DecodedImage::LoadMemory(std::span<const unsigned char> bytes, bool flip_vertically, int requested_channels)
{
	Reset();
	if (requested_channels < 0 || requested_channels > 4) return false;
	if (bytes.empty() || bytes.size() > static_cast<size_t>((std::numeric_limits<int>::max)())) return false;
	int width = 0, height = 0, channels = 0;
	if (!stbi_info_from_memory(bytes.data(), static_cast<int>(bytes.size()), &width, &height, &channels) || width <= 0 || height <= 0 ||
		static_cast<uint64_t>(width) * static_cast<uint64_t>(height) > 64ull * 1024 * 1024) return false;
	stbi_set_flip_vertically_on_load_thread(flip_vertically ? 1 : 0);
	m_pixels.reset(stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &m_width, &m_height, &m_channels, requested_channels));
	if (!m_pixels) Reset();
	else if (requested_channels) m_channels = requested_channels;
	return static_cast<bool>(m_pixels);
}

bool DecodedImage::LoadRGBA(std::span<const unsigned char> pixels, uint32_t width, uint32_t height)
{
	Reset();
	const uint64_t size = static_cast<uint64_t>(width) * height * 4;
	if (!width || !height || size > 256ull * 1024 * 1024 || size != pixels.size() || width > INT_MAX || height > INT_MAX) return false;
	m_pixels.reset(static_cast<unsigned char*>(std::malloc(static_cast<size_t>(size))));
	if (!m_pixels) return false;
	const size_t row = static_cast<size_t>(width) * 4;
	for (uint32_t y = 0; y < height; ++y)
		std::memcpy(m_pixels.get() + y * row, pixels.data() + (height - y - 1) * row, row);
	m_width = static_cast<int>(width); m_height = static_cast<int>(height); m_channels = 4;
	return true;
}
