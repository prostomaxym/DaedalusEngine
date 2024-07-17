#pragma once

#include <memory>
#include <vector>

namespace Daedalus {

	enum class FramebufferTextureFormat
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

	struct FramebufferTextureSpecification
	{
		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(FramebufferTextureFormat format)
			: texture_format(format) {}

		FramebufferTextureFormat texture_format = FramebufferTextureFormat::None;
	};

	struct FramebufferAttachmentSpecification
	{
		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
			: attachments(attachments) {}

		std::vector<FramebufferTextureSpecification> attachments;
	};

	struct FramebufferSpecification
	{
		uint32_t width = 0, height = 0;
		FramebufferAttachmentSpecification attachments;
		uint32_t samples = 1;
		int layers = -1;

		bool swap_chain_target = false;
	};

	class Framebuffer
	{
	public:
		static void CopyFramebuffer(unsigned int src_id, unsigned int dest_id, int width, int height);
		static void CopyDepthFramebuffer(unsigned int src_id, unsigned int dest_id, int width, int height);
		static void CopyColorFramebuffer(unsigned int src_id, unsigned int dest_id, int width, int height);
		static void CopyColorAttachment(unsigned int src_id, unsigned int dest_id, int width, int height, int src_attach_id, int dest_attach_id);

	public:
		virtual ~Framebuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) = 0;

		virtual void ClearAttachment(uint32_t attachmentIndex, int value) = 0;

		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const = 0;
		virtual uint32_t GetDepthAttachmentID() const = 0;
		virtual uint32_t GetID() const = 0;

		virtual const FramebufferSpecification& GetSpecification() const = 0;

		static std::shared_ptr<Framebuffer> Create(const FramebufferSpecification& spec);
	};
}
