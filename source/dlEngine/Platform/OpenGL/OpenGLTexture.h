#pragma once

#include "Renderer/Resourses/Texture.h"

#include <glad/glad.h>

namespace Daedalus {

	class OpenGLTexture2D final : public Texture2D
	{
	public:
		static bool IsDepthFormat(ColorFormat format)
		{
			switch (format)
			{
			case ColorFormat::Depth:  return true;
			}

			return false;
		}

		static GLenum DaedalusTextureFormatToGL(ColorFormat format)
		{
			switch (format)
			{
			case ColorFormat::RGBA8:        return GL_RGBA8;
			case ColorFormat::RGBA16F:      return GL_RGBA16F;
			case ColorFormat::RGBA32U:      return GL_RGBA;
			case ColorFormat::RED16F:       return GL_R16F;
			case ColorFormat::RED_INTEGER:  return GL_RED_INTEGER;
			case ColorFormat::RED_FLOAT:    return GL_RED;
			case ColorFormat::Depth:		return GL_DEPTH24_STENCIL8;
			}

			return 0;
		}

	public:
		OpenGLTexture2D(uint32_t width, uint32_t height, int channels = 4);
		OpenGLTexture2D(const std::filesystem::path& path);
		OpenGLTexture2D(unsigned char* data, int width, int heith, int channels);
		OpenGLTexture2D(float* data, int width, int heith, int channels);
		virtual ~OpenGLTexture2D();

		uint32_t GetWidth() const override { return m_width; }
		uint32_t GetHeight() const override { return m_height; }
		uint32_t GetRendererID() const override { return m_rendererID; }

		const std::filesystem::path& GetPath() const override { return m_path; }

		void SetData(void* data, uint32_t size) override;

		void Bind(uint32_t slot = 0) const override;

		bool IsLoaded() const override { return m_is_loaded; }

		bool operator==(const Texture& other) const override
		{
			return m_rendererID == other.GetRendererID();
		}

		static void BindTextureImpl(uint32_t ID, uint32_t slot);
		static void BindTextureImageImpl(uint32_t slot, uint32_t ID, ColorFormat format, bool read, bool write);

	private:
		std::filesystem::path m_path;
		bool m_is_loaded = false;
		uint32_t m_width{ 0 }, m_height{ 0 };
		uint32_t m_rendererID{ 0 };
		GLenum m_internal_format{ 0 }, m_data_format{ 0 };
	};


	class OpenGLTextureCubemap final : public TextureCubemap
	{
	public:
		OpenGLTextureCubemap(const std::vector<std::string>& faces);
		~OpenGLTextureCubemap();

		uint32_t GetRendererID() const override { return m_rendererID; }

		void Bind(uint32_t slot = 0) const override;

		bool operator==(const Texture& other) const override
		{
			return m_rendererID == other.GetRendererID();
		}

	private:
		uint32_t m_rendererID{ 0 };
	};
}
