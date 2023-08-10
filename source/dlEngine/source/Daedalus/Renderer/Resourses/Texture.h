#pragma once

#include <memory>
#include <string>

namespace Daedalus {

	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const { return 0; };
		virtual uint32_t GetHeight() const { return 0; };
		virtual uint32_t GetRendererID() const = 0;

		virtual const std::string& GetPath() const { return {}; }

		virtual void SetData(void* data, uint32_t size) {};

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual bool IsLoaded() const { return true; };

		virtual bool operator==(const Texture& other) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		static std::shared_ptr<Texture2D> Create(uint32_t width, uint32_t height);
		static std::shared_ptr<Texture2D> Create(const std::string& path);
		static std::shared_ptr<Texture2D> Create(unsigned char* data, int width, int height, int channels);

		static void BindTexture(uint32_t ID, uint32_t slot);
	};

	class TextureCubemap : public Texture
	{
	public:
		static std::shared_ptr<TextureCubemap> Create(std::vector<std::string>& faces);
	};

}
