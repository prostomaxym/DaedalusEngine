#pragma once

#include <memory>
#include <string>
#include <filesystem>

namespace Daedalus {

	class Texture
	{
	public:
		enum class ColorFormat
		{
			None = 0,

			// Color
			RGBA8,
			RGBA16F,
			RGBA32U,
			RED_INTEGER,
			RED_FLOAT,
			RED16F,
			Depth
		};

	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const { return 0; };
		virtual uint32_t GetHeight() const { return 0; };
		virtual uint32_t GetRendererID() const = 0;

		virtual const std::filesystem::path& GetPath() const;

		virtual void SetData(void* data, uint32_t size) {};

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual bool IsLoaded() const { return true; };

		virtual bool operator==(const Texture& other) const = 0;
	};

	class ResourceManager;

	class Texture2D : public Texture
	{
		friend class ResourceManager;

		static std::shared_ptr<Texture2D> Create(uint32_t width, uint32_t height, int channels = 4);
		static std::shared_ptr<Texture2D> Create(const std::filesystem::path& path);
		static std::shared_ptr<Texture2D> Create(unsigned char* data, int width, int height, int channels);
		static std::shared_ptr<Texture2D> Create(float* data, int width, int height, int channels);
	public:

		static void BindTexture(uint32_t ID, uint32_t slot);
		static void BindTextureImage(uint32_t ID, uint32_t slot, ColorFormat format, bool read, bool write);
	};

	class TextureCubemap : public Texture
	{
		friend class ResourceManager;
		static std::shared_ptr<TextureCubemap> Create(const std::vector<std::string>& faces);
	};
}