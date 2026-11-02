#include "dlpch.h"
#include "Platform/OpenGL/OpenGLTexture.h"
#include "Renderer/Resources/DecodedImage.h"
#include "OpenGLTextureBinding.h"

#ifdef DL_PLATFORM_LINUX
	#define STBI_NO_SIMD //Have no idea why, but linux gcc fails to compile with simd
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace Daedalus;


namespace
{
	uint32_t BytesPerPixel(GLenum internal_format)
	{
		switch (internal_format)
		{
		case GL_RGBA8:    return 4;
		case GL_RGB8:     return 3;
		case GL_R8:       return 1;
		case GL_RG8:      return 2;
		case GL_RGBA32F:  return 16;
		case GL_RGB32F:   return 12;
		default:          return 0;
		}
	}
}

OpenGLTextureCubemap::OpenGLTextureCubemap(const std::vector<std::shared_ptr<DecodedImage>>& faces)
{
	if (faces.size() != 6 || !faces.front()) return;
	const auto width = faces.front()->GetWidth();
	for (const auto& face : faces)
		if (!face || !face->GetPixels() || face->GetWidth() != width || face->GetHeight() != width || face->GetChannels() != 3) return;
	glGenTextures(1, &m_rendererID);
	if (!m_rendererID) return;
	ScopedTextureBinding binding(GL_TEXTURE_CUBE_MAP, m_rendererID);
	glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGB8, width, width);
	GLint allocated_width = 0;
	glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_WIDTH, &allocated_width);
	if (allocated_width != width) return;
	GLint alignment = 4;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (size_t face = 0; face < faces.size(); ++face)
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(face), 0, 0, 0, width, width, GL_RGB, GL_UNSIGNED_BYTE, faces[face]->GetPixels());
	glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	for (const auto parameter : { GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R }) glTexParameteri(GL_TEXTURE_CUBE_MAP, parameter, GL_CLAMP_TO_EDGE);
	m_width = static_cast<uint32_t>(width); m_height = m_width; m_faces_loaded = 6;
}

bool OpenGLTextureCubemap::SwapStorage(TextureCubemap& replacement)
{
	auto* texture = dynamic_cast<OpenGLTextureCubemap*>(&replacement);
	if (!texture || !texture->IsLoaded()) return false;
	std::swap(m_rendererID, texture->m_rendererID); std::swap(m_width, texture->m_width);
	std::swap(m_height, texture->m_height); std::swap(m_faces_loaded, texture->m_faces_loaded);
	return true;
}

namespace
{
	std::vector<unsigned char> ConvertToRGB(unsigned char* grayscaleImage, int width, int height)
	{
		// Allocate memory for the new RGB image
		std::vector<unsigned char> rgbImage(width * height * 3);

		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				// Get the grayscale pixel value (single-channel)
				unsigned char grayscalePixel = grayscaleImage[y * width + x];

				// Set the RGB pixel values to the grayscale value (duplicate to R, G, and B channels)
				rgbImage[(y * width + x) * 3 + 0] = grayscalePixel; // Red channel
				rgbImage[(y * width + x) * 3 + 1] = grayscalePixel; // Green channel
				rgbImage[(y * width + x) * 3 + 2] = grayscalePixel; // Blue channel
			}
		}

		return rgbImage;
	}
}

OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height, int channels)
	: m_width(width), m_height(height)
{

	if (channels == 4)
	{
		m_internal_format = GL_RGBA8;
		m_data_format = GL_RGBA;
	}
	else if (channels == 3)
	{
		m_internal_format = GL_RGB8;
		m_data_format = GL_RGB;
	}
	else if (channels == 1)
	{
		m_internal_format = GL_RGB8;
		m_data_format = GL_RGB;
	}

	m_internal_format = GL_RGBA8;
	m_data_format = GL_RGBA;

	glGenTextures(1, &m_rendererID);
	ScopedTextureBinding binding(GL_TEXTURE_2D, m_rendererID);
	glTexStorage2D(GL_TEXTURE_2D, 1, m_internal_format, m_width, m_height);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

OpenGLTexture2D::OpenGLTexture2D(const std::filesystem::path& path)
	: m_path(path)
{
	int width, height, channels;
	stbi_set_flip_vertically_on_load_thread(1);
	stbi_uc* data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
	auto img_data = data;
	std::vector<unsigned char> converted_data;

	if (img_data)
	{
		m_is_loaded = true;

		m_width = width;
		m_height = height;

		GLenum internalFormat = 0, dataFormat = 0;
		if (channels == 4)
		{
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
		}
		else if (channels == 3)
		{
			internalFormat = GL_RGB8;
			dataFormat = GL_RGB;
		}
		else if (channels == 1)
		{
			converted_data = ConvertToRGB(data, m_width, m_height);
			img_data = converted_data.data();
			internalFormat = GL_RGB8;
			dataFormat = GL_RGB;
		}

		m_internal_format = internalFormat;
		m_data_format = dataFormat;

		glGenTextures(1, &m_rendererID);
		ScopedTextureBinding binding(GL_TEXTURE_2D, m_rendererID);
		glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, m_width, m_height);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, dataFormat, GL_UNSIGNED_BYTE, img_data);
		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(data);
	}
	else
	{
		Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Failed to load texture");
	}	
}

OpenGLTexture2D::OpenGLTexture2D(unsigned char* data, int width, int height, int channels)
{
	if (!data || width <= 0 || height <= 0 || channels < 1 || channels > 4) return;
	const GLenum formats[]{ GL_RED, GL_RG, GL_RGB, GL_RGBA };
	const GLenum storage[]{ GL_R8, GL_RG8, GL_RGB8, GL_RGBA8 };
	m_internal_format = storage[channels - 1]; m_data_format = formats[channels - 1];
	m_width = static_cast<uint32_t>(width); m_height = static_cast<uint32_t>(height);
	glGenTextures(1, &m_rendererID);
	if (!m_rendererID) return;
	ScopedTextureBinding binding(GL_TEXTURE_2D, m_rendererID);
	glTexStorage2D(GL_TEXTURE_2D, 1, m_internal_format, width, height);
	GLint allocated_width = 0;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &allocated_width);
	if (allocated_width != width) return;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	if (channels <= 2)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, channels == 2 ? GL_GREEN : GL_ONE);
	}
	GLint alignment = 4;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, m_data_format, GL_UNSIGNED_BYTE, data);
	glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
	m_is_loaded = true;
}

bool OpenGLTexture2D::SwapStorage(Texture2D& replacement)
{
	auto* texture = dynamic_cast<OpenGLTexture2D*>(&replacement);
	if (!texture || !texture->m_is_loaded) return false;
	std::swap(m_rendererID, texture->m_rendererID);
	std::swap(m_width, texture->m_width); std::swap(m_height, texture->m_height);
	std::swap(m_internal_format, texture->m_internal_format); std::swap(m_data_format, texture->m_data_format);
	std::swap(m_is_loaded, texture->m_is_loaded);
	return true;
}

OpenGLTexture2D::OpenGLTexture2D(float* data, int width, int height, int channels)
{
	if (data)
	{
		m_is_loaded = true;

		m_width = width;
		m_height = height;

		GLenum internalFormat = 0, dataFormat = 0;
		if (channels == 4)
		{
			internalFormat = GL_RGBA32F;
			dataFormat = GL_RGBA;
		}
		else if (channels == 3)
		{
			internalFormat = GL_RGB32F;
			dataFormat = GL_RGB;
		}
		else
		{
			DL_ASSERT(false, Log::Categories::Renderer, "Not Implemented");
		}

		m_internal_format = internalFormat;
		m_data_format = dataFormat;

		glGenTextures(1, &m_rendererID);
		ScopedTextureBinding binding(GL_TEXTURE_2D, m_rendererID);
		glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, m_width, m_height);

		// Only one immutable level exists; RGB32F mipmap generation is also invalid on GLES.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, dataFormat, GL_FLOAT, data);
	}
	else
	{
		Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Failed to load texture");
	}
}

uint64_t OpenGLTexture2D::GetMemorySizeBytes() const
{
	// Storage is always allocated with a single mip level
	return static_cast<uint64_t>(m_width) * m_height * BytesPerPixel(m_internal_format);
}

const char* OpenGLTexture2D::GetFormatName() const
{
	switch (m_internal_format)
	{
	case GL_RGBA8:    return "RGBA8";
	case GL_RGB8:     return "RGB8";
	case GL_R8:       return "R8";
	case GL_RG8:      return "RG8";
	case GL_RGBA32F:  return "RGBA32F";
	case GL_RGB32F:   return "RGB32F";
	default:          return "unknown";
	}
}

OpenGLTexture2D::~OpenGLTexture2D()
{
	glDeleteTextures(1, &m_rendererID);
}

void OpenGLTexture2D::SetData(void* data, uint32_t size)
{
	uint32_t bpp = m_data_format == GL_RGBA ? 4 : 3;
	DL_ASSERT(size == m_width * m_height * bpp, Log::Categories::Renderer, "Data must be entire texture!");
	ScopedTextureBinding binding(GL_TEXTURE_2D, m_rendererID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, m_data_format, GL_UNSIGNED_BYTE, data);
}

void OpenGLTexture2D::Bind(uint32_t slot) const
{
	BindTextureUnit(slot, GL_TEXTURE_2D, m_rendererID);
}

void OpenGLTexture2D::BindTextureImpl(uint32_t ID, uint32_t slot)
{
	BindTextureUnit(slot, GL_TEXTURE_2D, ID);
}

void OpenGLTexture2D::BindTextureArrayImpl(uint32_t ID, uint32_t slot)
{
	BindTextureUnit(slot, GL_TEXTURE_2D_ARRAY, ID);
}

void OpenGLTexture2D::BindTextureImageImpl(uint32_t slot, uint32_t ID, ColorFormat format, bool read, bool write)
{
	GLenum access = 0;

	if (read && write)
		access = GL_READ_WRITE;
	else if (write)
		access = GL_WRITE_ONLY;
	else
		access = GL_READ_ONLY;

	glBindImageTexture(slot, ID, 0, GL_FALSE, 0, access, DaedalusTextureFormatToGL(format));
}

OpenGLTextureCubemap::OpenGLTextureCubemap(const std::vector<std::string>& faces)
{
	if (faces.size() != 6) return;
	glGenTextures(1, &m_rendererID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_rendererID);
	GLint unpack_alignment = 0;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack_alignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load_thread(0);
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, STBI_rgb);
		if (data && width == height && (i == 0 || (static_cast<uint32_t>(width) == m_width && static_cast<uint32_t>(height) == m_height)))
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			m_width = static_cast<uint32_t>(width);
			m_height = static_cast<uint32_t>(height);
			m_faces_loaded++;
		}
		else
		{
			Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Failed to load texture from path: {}", faces[i]);
		}

		stbi_image_free(data);
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

uint64_t OpenGLTextureCubemap::GetMemorySizeBytes() const
{
	return static_cast<uint64_t>(m_width) * m_height * BytesPerPixel(GL_RGB8) * m_faces_loaded;
}

OpenGLTextureCubemap::~OpenGLTextureCubemap()
{
	glDeleteTextures(1, &m_rendererID);
}

void OpenGLTextureCubemap::Bind(uint32_t slot) const
{
	BindTextureUnit(slot, GL_TEXTURE_CUBE_MAP, m_rendererID);
}
