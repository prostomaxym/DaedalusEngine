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

			//float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			//glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, TextureTarget(multisampled), id, 0);
	}

	static bool IsDepthFormat(FramebufferTextureFormat format)
	{
		switch (format)
		{
			case FramebufferTextureFormat::Depth:  return true;
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

		Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unsupported FramebufferTextureFormat");
		return 0;
	}

}

OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& spec)
	: m_specification(spec)
{
	for (auto spec : m_specification.attachments.attachments)
	{
		if (!IsDepthFormat(spec.texture_format))
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

	bool multisample = m_specification.samples > 1;

	// Attachments
	if (m_color_attachment_specifications.size())
	{
		m_color_attachments.resize(m_color_attachment_specifications.size());
		CreateTextures(multisample, m_color_attachments.data(), m_color_attachments.size());

		for (size_t i = 0; i < m_color_attachments.size(); i++)
		{
			BindTexture(multisample, m_color_attachments[i]);
			switch (m_color_attachment_specifications[i].texture_format)
			{
				case FramebufferTextureFormat::RGBA8:
					AttachColorTexture(m_color_attachments[i], m_specification.samples, GL_RGBA8, GL_RGBA, m_specification.width, m_specification.height, i);
					break;
				case FramebufferTextureFormat::RED_INTEGER:
					AttachColorTexture(m_color_attachments[i], m_specification.samples, GL_R32I, GL_RED_INTEGER, m_specification.width, m_specification.height, i);
					break;
			}
		}
	}

	if (m_depth_attachment_specification.texture_format != FramebufferTextureFormat::None)
	{
		CreateTextures(multisample, &m_depth_attachment, 1);
		BindTexture(multisample, m_depth_attachment);
		switch (m_depth_attachment_specification.texture_format)
		{
			case FramebufferTextureFormat::Depth:
				AttachDepthTexture(m_depth_attachment, m_specification.samples, GL_DEPTH_COMPONENT32F, GL_DEPTH_ATTACHMENT, m_specification.width, m_specification.height);
				break;
		}
	}

	if (m_color_attachments.size() > 1)
	{
		DL_ASSERT(m_color_attachments.size() <= 4, Log::Categories::Renderer, "incorrect color attachment size");
		GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(m_color_attachments.size(), buffers);
	}
	else if (m_color_attachments.empty())
	{
		// Only depth-pass
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}

	DL_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, Log::Categories::Renderer, "Framebuffer is incomplete!");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFramebuffer::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);
	glViewport(0, 0, m_specification.width, m_specification.height);
}

void OpenGLFramebuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
	{
		Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "Attempted to rezize framebuffer to {0}, {1}", width, height);
		return;
	}
	m_specification.width = width;
	m_specification.height = height;
		
	Invalidate();
}

int OpenGLFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
{
	DL_ASSERT(attachmentIndex < m_color_attachments.size(), Log::Renderer, "Incorret attachment index");

	glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
	int pixelData;
	glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
	return pixelData;

}

void OpenGLFramebuffer::ClearAttachment(uint32_t attachmentIndex, int value)
{
	DL_ASSERT(attachmentIndex < m_color_attachments.size(), Log::Renderer, "Incorret attachment index");

	auto& spec = m_color_attachment_specifications[attachmentIndex];
	glClearTexImage(m_color_attachments[attachmentIndex], 0,
		DaedalusFBTextureFormatToGL(spec.texture_format), GL_INT, &value);
}
