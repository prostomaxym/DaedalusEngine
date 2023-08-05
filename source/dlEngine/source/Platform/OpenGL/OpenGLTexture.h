#pragma once

#include "Daedalus/Renderer/Resourses/Texture.h"

#include <glad/glad.h>

namespace Daedalus {

	class OpenGLTexture2D final : public Texture2D
	{
	public:
		OpenGLTexture2D(uint32_t width, uint32_t height);
		OpenGLTexture2D(const std::string& path);	
		OpenGLTexture2D(unsigned char* data, int width, int heith, int channels);
		~OpenGLTexture2D();

		uint32_t GetWidth() const override { return m_width; }
		uint32_t GetHeight() const override { return m_height; }
		uint32_t GetRendererID() const override { return m_rendererID; }

		const std::string& GetPath() const override { return m_path; }

		void SetData(void* data, uint32_t size) override;

		void Bind(uint32_t slot = 0) const override;

		bool IsLoaded() const override { return m_is_loaded; }

		bool operator==(const Texture& other) const override
		{
			return m_rendererID == other.GetRendererID();
		}

	private:
		std::string m_path;
		bool m_is_loaded = false;
		uint32_t m_width{ 0 }, m_height{ 0 };
		uint32_t m_rendererID{ 0 };
		GLenum m_internal_format{ 0 }, m_data_format{ 0 };
	};


	class OpenGLTextureCubemap final : public TextureCubemap
	{
	public:
		OpenGLTextureCubemap(std::vector<std::string>& faces);
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
