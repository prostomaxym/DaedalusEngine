#include "dlpch.h"
#include "Platform/OpenGL/OpenGLTexture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace Daedalus;


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

OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height)
	: m_width(width), m_height(height)
{

	m_internal_format = GL_RGBA8;
	m_data_format = GL_RGBA;

	glCreateTextures(GL_TEXTURE_2D, 1, &m_rendererID);
	glTextureStorage2D(m_rendererID, 1, m_internal_format, m_width, m_height);

	glTextureParameteri(m_rendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_rendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

OpenGLTexture2D::OpenGLTexture2D(const std::string& path)
	: m_path(path)
{
	int width, height, channels;
	stbi_set_flip_vertically_on_load(1);
	stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
			
	if (data)
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
			internalFormat = GL_RGBA8;
			dataFormat = GL_STENCIL_INDEX;
		}

		m_internal_format = internalFormat;
		m_data_format = dataFormat;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_rendererID);
		glTextureStorage2D(m_rendererID, 1, internalFormat, m_width, m_height);

		glTextureParameteri(m_rendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(m_rendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTextureSubImage2D(m_rendererID, 0, 0, 0, m_width, m_height, dataFormat, GL_UNSIGNED_BYTE, data);
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
	if (data)
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
			internalFormat = GL_RGBA8;
			dataFormat = GL_STENCIL_INDEX;
		}

		m_internal_format = internalFormat;
		m_data_format = dataFormat;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_rendererID);
		glTextureStorage2D(m_rendererID, 1, internalFormat, m_width, m_height);

		glTextureParameteri(m_rendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(m_rendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTextureSubImage2D(m_rendererID, 0, 0, 0, m_width, m_height, dataFormat, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Failed to load texture");
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
	glTextureSubImage2D(m_rendererID, 0, 0, 0, m_width, m_height, m_data_format, GL_UNSIGNED_BYTE, data);
}

void OpenGLTexture2D::Bind(uint32_t slot) const
{
	glBindTextureUnit(slot, m_rendererID);
}

void OpenGLTexture2D::BindTextureImpl(uint32_t ID, uint32_t slot)
{
	glBindTextureUnit(slot, ID);
}

OpenGLTextureCubemap::OpenGLTextureCubemap(std::vector<std::string>& faces)
{
	glGenTextures(1, &m_rendererID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_rendererID);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(0);
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Failed to load texture from path: " + faces[i]);
		}

		stbi_image_free(data);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

OpenGLTextureCubemap::~OpenGLTextureCubemap()
{
	glDeleteTextures(1, &m_rendererID);
}

void OpenGLTextureCubemap::Bind(uint32_t slot) const
{
	glBindTextureUnit(slot, m_rendererID);
}
