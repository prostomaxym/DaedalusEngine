#include "dlpch.h"
#include "Platform/OpenGL/OpenGLFrameBuffer.h"

#include <glad/glad.h>

using namespace Daedalus;

static const uint32_t s_MaxFramebufferSize = 8192;

namespace {

	static GLenum TextureTarget(bool multisampled)
	{
		return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
	}

	static void CreateTextures(bool multisampled, uint32_t* outID, uint32_t count)
	{
		glCreateTextures(TextureTarget(multisampled), count, outID);
	}

	static void BindTexture(bool multisampled, uint32_t id)
	{
		glBindTexture(TextureTarget(multisampled), id);
	}

	static void AttachColorTexture(uint32_t id, int samples, GLenum internalFormat, GLenum format, uint32_t width, uint32_t height, int index)
	{
		bool multisampled = samples > 1;
		if (multisampled)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget(multisampled), id, 0);
	}

	static void AttachDepthTexture(uint32_t id, int samples, GLenum format, GLenum attachmentType, uint32_t width, uint32_t height)
	{
		bool multisampled = samples > 1;
		if (multisampled)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
		}
		else
		{
			glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, TextureTarget(multisampled), id, 0);
	}

	static bool IsDepthFormat(FramebufferTextureFormat format)
	{
		switch (format)
		{
			case FramebufferTextureFormat::DEPTH24STENCIL8:  return true;
		}

		return false;
	}

	static GLenum DaedalusFBTextureFormatToGL(FramebufferTextureFormat format)
	{
		switch (format)
		{
			case FramebufferTextureFormat::RGBA8:       return GL_RGBA8;
			case FramebufferTextureFormat::RED_INTEGER: return GL_RED_INTEGER;
		}

		DL_CORE_ASSERT(false, "Unsupported FramebufferTextureFormat");
		return 0;
	}

}

OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& spec)
	: m_specification(spec)
{
	for (auto spec : m_specification.Attachments.Attachments)
	{
		if (!IsDepthFormat(spec.TextureFormat))
			m_color_attachment_specifications.emplace_back(spec);
		else
			m_depth_attachment_specification = spec;
	}

	Invalidate();
}

OpenGLFramebuffer::~OpenGLFramebuffer()
{
	glDeleteFramebuffers(1, &m_rendererID);
	glDeleteTextures(m_color_attachments.size(), m_color_attachments.data());
	glDeleteTextures(1, &m_depth_attachment);
}

void OpenGLFramebuffer::Invalidate()
{
	if (m_rendererID)
	{
		glDeleteFramebuffers(1, &m_rendererID);
		glDeleteTextures(m_color_attachments.size(), m_color_attachments.data());
		glDeleteTextures(1, &m_depth_attachment);
			
		m_color_attachments.clear();
		m_depth_attachment = 0;
	}

	glCreateFramebuffers(1, &m_rendererID);
	glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);

	bool multisample = m_specification.Samples > 1;

	// Attachments
	if (m_color_attachment_specifications.size())
	{
		m_color_attachments.resize(m_color_attachment_specifications.size());
		CreateTextures(multisample, m_color_attachments.data(), m_color_attachments.size());

		for (size_t i = 0; i < m_color_attachments.size(); i++)
		{
			BindTexture(multisample, m_color_attachments[i]);
			switch (m_color_attachment_specifications[i].TextureFormat)
			{
				case FramebufferTextureFormat::RGBA8:
					AttachColorTexture(m_color_attachments[i], m_specification.Samples, GL_RGBA8, GL_RGBA, m_specification.Width, m_specification.Height, i);
					break;
				case FramebufferTextureFormat::RED_INTEGER:
					AttachColorTexture(m_color_attachments[i], m_specification.Samples, GL_R32I, GL_RED_INTEGER, m_specification.Width, m_specification.Height, i);
					break;
			}
		}
	}

	if (m_depth_attachment_specification.TextureFormat != FramebufferTextureFormat::None)
	{
		CreateTextures(multisample, &m_depth_attachment, 1);
		BindTexture(multisample, m_depth_attachment);
		switch (m_depth_attachment_specification.TextureFormat)
		{
			case FramebufferTextureFormat::DEPTH24STENCIL8:
				AttachDepthTexture(m_depth_attachment, m_specification.Samples, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT, m_specification.Width, m_specification.Height);
				break;
		}
	}

	if (m_color_attachments.size() > 1)
	{
		DL_CORE_ASSERT(m_color_attachments.size() <= 4, "incorrect color attachment size");
		GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(m_color_attachments.size(), buffers);
	}
	else if (m_color_attachments.empty())
	{
		// Only depth-pass
		glDrawBuffer(GL_NONE);
	}

	DL_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFramebuffer::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);
	glViewport(0, 0, m_specification.Width, m_specification.Height);
}

void OpenGLFramebuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
	{
		DL_CORE_WARN("Attempted to rezize framebuffer to {0}, {1}", width, height);
		return;
	}
	m_specification.Width = width;
	m_specification.Height = height;
		
	Invalidate();
}

int OpenGLFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
{
	DL_CORE_ASSERT(attachmentIndex < m_color_attachments.size(), "Incorret attachment index");

	glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
	int pixelData;
	glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
	return pixelData;

}

void OpenGLFramebuffer::ClearAttachment(uint32_t attachmentIndex, int value)
{
	DL_CORE_ASSERT(attachmentIndex < m_color_attachments.size(), "Incorret attachment index");

	auto& spec = m_color_attachment_specifications[attachmentIndex];
	glClearTexImage(m_color_attachments[attachmentIndex], 0,
		DaedalusFBTextureFormatToGL(spec.TextureFormat), GL_INT, &value);
}
