#pragma once

#include "Macros.h"
#include "Renderer/Primitives/FrameBuffer.h"

namespace Daedalus {

	class OpenGLFramebuffer final : public Framebuffer
	{
	public:
		static void CopyFramebufferImpl(unsigned int src_id, unsigned int dest_id, int width, int height);
		static void CopyDepthFramebufferImpl(unsigned int src_id, unsigned int dest_id, int width, int height);
		static void CopyColorFramebufferImpl(unsigned int src_id, unsigned int dest_id, int width, int height);
		static void CopyColorAttachmentImpl(unsigned int src_id, unsigned int dest_id, int width, int height, int src_attach_id, int dest_attach_id);

	public:
		OpenGLFramebuffer(const FramebufferSpecification& spec);
		~OpenGLFramebuffer();

		void Invalidate();

		static void BindDefault();
		void Bind() override;
		void Unbind() override;

		void BindDrawColorAttachment(int attachment_num) override;

		void Resize(uint32_t width, uint32_t height) override;
		int ReadPixel(uint32_t attachmentIndex, int x, int y) override;

		const FramebufferSpecification& GetSpecification() const override { return m_specification; }

		void ClearAttachment(uint32_t attachmentIndex, int value) override;

		uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override { return m_color_attachments[index]; }
		uint32_t GetDepthAttachmentID() const { return m_depth_attachment; }
		uint32_t GetID() const override { return m_rendererID; };

	private:
		uint32_t m_rendererID = 0;
		FramebufferSpecification m_specification;

		std::vector<FramebufferTextureSpecification> m_color_attachment_specifications;
		FramebufferTextureSpecification m_depth_attachment_specification = FramebufferTextureFormat::None;

		std::vector<uint32_t> m_color_attachments;
		uint32_t m_depth_attachment = 0;
	};

}
