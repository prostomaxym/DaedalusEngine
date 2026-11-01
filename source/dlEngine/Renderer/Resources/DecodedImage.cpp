#include "dlpch.h"
#include "DecodedImage.h"

#include <limits>
#include <stb_image.h>

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

bool DecodedImage::LoadMemory(std::span<const unsigned char> bytes, bool flip_vertically)
{
	Reset();
	if (bytes.empty() || bytes.size() > static_cast<size_t>((std::numeric_limits<int>::max)())) return false;
	stbi_set_flip_vertically_on_load_thread(flip_vertically ? 1 : 0);
	m_pixels.reset(stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &m_width, &m_height, &m_channels, 0));
	if (!m_pixels) Reset();
	return static_cast<bool>(m_pixels);
}
