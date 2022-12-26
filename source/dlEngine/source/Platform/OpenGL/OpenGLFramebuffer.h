#pragma once

#include "Daedalus/Core/Core.h"
#include "Daedalus/Renderer/Primitives/Framebuffer.h"

namespace Daedalus {

class OpenGLFramebuffer final: public Framebuffer
{
public:
	OpenGLFramebuffer(const FramebufferSpecification& spec);
	~OpenGLFramebuffer();

	void Invalidate();

	void Bind() override;
	void Unbind() override;

	void Resize(uint32_t width, uint32_t height) override;
	int ReadPixel(uint32_t attachmentIndex, int x, int y) override;

	void ClearAttachment(uint32_t attachmentIndex, int value) override;

	uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override {	return m_color_attachments[index]; }

	const FramebufferSpecification& GetSpecification() const override { return m_specification; }

private:
	uint32_t m_rendererID = 0;
	FramebufferSpecification m_specification;

	std::vector<FramebufferTextureSpecification> m_color_attachment_specifications;
	FramebufferTextureSpecification m_depth_attachment_specification = FramebufferTextureFormat::None;

	std::vector<uint32_t> m_color_attachments;
	uint32_t m_depth_attachment = 0;
};

}
