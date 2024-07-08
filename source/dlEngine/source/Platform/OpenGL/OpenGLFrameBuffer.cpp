#include "dlpch.h"
#include "Platform/OpenGL/OpenGLFrameBuffer.h"

#include <glad/glad.h>

using namespace Daedalus;

namespace {

	GLint GetMaxTextureSize()
	{
		GLint max_texture_size;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

		return max_texture_size;
	}
	
	GLenum TextureTarget(bool multisampled, bool multilayer)
	{
		GLenum tex_type;
		if (multisampled)
		{
			if (multilayer)
			{
				tex_type = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
			}
			else
			{
				tex_type = GL_TEXTURE_2D_MULTISAMPLE;
			}
				
		}	
		else
		{
			if (multilayer)
			{
				tex_type = GL_TEXTURE_2D_ARRAY;
			}
			else
			{
				tex_type = GL_TEXTURE_2D;
			}	
		}

		return tex_type;
	}

	void CreateTextures(bool multisampled, bool multilayer, uint32_t* outID, uint32_t count)
	{
		glCreateTextures(TextureTarget(multisampled, multilayer), count, outID);
	}

	void BindTexture(bool multisampled, bool multilayer, uint32_t id)
	{
		glBindTexture(TextureTarget(multisampled, multilayer), id);
	}

	void AttachColorTexture(uint32_t id, int samples, int layers, GLenum internalFormat, GLenum format, uint32_t width, uint32_t height, int index)
	{
		bool multisampled = samples > 1;
		bool multilayer = layers > 0;

		if (multisampled && multilayer)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, samples, internalFormat, width, height, GL_FALSE);
		}
		else if (multisampled)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
		}
		else if (multilayer)
		{
			glTexImage3D(
				GL_TEXTURE_2D_ARRAY,
				0,
				internalFormat,
				width,
				height,
				layers,
				0,
				format,
				GL_UNSIGNED_BYTE,
				nullptr);

			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
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

		if (multilayer)
			glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget(multisampled, true), id, 0, 0);
		else
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget(multisampled, false), id, 0);
	}

	void AttachDepthTexture(uint32_t id, int samples, int layers, GLenum format, GLenum attachmentType, uint32_t width, uint32_t height)
	{
		bool multisampled = samples > 1;
		bool multilayer = layers > 0;
		if (multisampled && multilayer)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, samples, format, width, height, GL_FALSE);
		}
		else if (multisampled)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
		}
		else if (multilayer)
		{
			glTexImage3D(
				GL_TEXTURE_2D_ARRAY,
				0,
				format,
				width,
				height,
				layers,
				0,
				GL_DEPTH_COMPONENT,
				GL_FLOAT,
				nullptr);

			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

			float border_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
		}
		else
		{
			glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			float border_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
		}

		if (multilayer)
			glFramebufferTexture(GL_FRAMEBUFFER, attachmentType, id, 0);
		else
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, TextureTarget(multisampled, false), id, 0);
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
	GLint max_texture_size = GetMaxTextureSize();

	if (m_specification.width == 0 || m_specification.height == 0 || m_specification.width > max_texture_size || m_specification.height > max_texture_size)
	{
		Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "Attempted to rezize framebuffer to {0}, {1}", m_specification.width, m_specification.height);
		return;
	}

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
	bool multilayer = m_specification.layers > 0;

	// Attachments
	if (m_color_attachment_specifications.size())
	{
		m_color_attachments.resize(m_color_attachment_specifications.size());
		CreateTextures(multisample, multilayer, m_color_attachments.data(), m_color_attachments.size());

		for (size_t i = 0; i < m_color_attachments.size(); i++)
		{
			BindTexture(multisample, multilayer, m_color_attachments[i]);
			switch (m_color_attachment_specifications[i].texture_format)
			{
				case FramebufferTextureFormat::RGBA8:
					AttachColorTexture(m_color_attachments[i], m_specification.samples, m_specification.layers, GL_RGBA8, GL_RGBA, m_specification.width, m_specification.height, i);
					break;
				case FramebufferTextureFormat::RED_INTEGER:
					AttachColorTexture(m_color_attachments[i], m_specification.samples, m_specification.layers, GL_R32I, GL_RED_INTEGER, m_specification.width, m_specification.height, i);
					break;
			}
		}
	}

	if (m_depth_attachment_specification.texture_format != FramebufferTextureFormat::None)
	{
		CreateTextures(multisample, multilayer , &m_depth_attachment, 1);
		BindTexture(multisample, multilayer, m_depth_attachment);
		switch (m_depth_attachment_specification.texture_format)
		{
			case FramebufferTextureFormat::Depth:
				AttachDepthTexture(m_depth_attachment, m_specification.samples, m_specification.layers, GL_DEPTH_COMPONENT32F, GL_DEPTH_ATTACHMENT, m_specification.width, m_specification.height);
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
	GLint max_texture_size = GetMaxTextureSize();

	if (width == 0 || height == 0 || width > max_texture_size || height > max_texture_size)
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
