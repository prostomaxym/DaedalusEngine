#include "dlpch.h"
#include "Platform/OpenGL/OpenGLFrameBuffer.h"

#include <glad/glad.h>

using namespace Daedalus;

namespace
{
	// Do not know how to make it platform independent, so will cover only common scenario
	GLint GetMainFramebufferFormat(int current_id)
	{
		GLint depth_size;
		GLint stencil_size;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depth_size);
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencil_size);

		
		GLint format = GL_DEPTH24_STENCIL8; // For my RTX 3070Ti this is default, so I will assume it is common default
		if (stencil_size == 8)
		{
			if (depth_size == 24)
				format = GL_DEPTH24_STENCIL8;
			else
				format = GL_DEPTH32F_STENCIL8;
		}
		else
		{
			if(depth_size == 32)
				format = GL_DEPTH_COMPONENT32;
			else if (depth_size == 24)
				format = GL_DEPTH_COMPONENT24;
			else if (depth_size == 16)
				format = GL_DEPTH_COMPONENT16;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, current_id);

		return format;	
	}

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

	void AttachColorTexture(uint32_t id, int samples, int layers, GLenum internalFormat, GLenum format, GLenum type, uint32_t width, uint32_t height, int index)
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
				type,
				nullptr);

			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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

	bool IsDepthFormat(FramebufferTextureFormat format)
	{
		switch (format)
		{
			case FramebufferTextureFormat::Depth:  return true;
		}

		return false;
	}

	GLenum DaedalusFBTextureFormatToGL(FramebufferTextureFormat format)
	{
		switch (format)
		{
			case FramebufferTextureFormat::RGBA8:        return GL_RGBA8;
			case FramebufferTextureFormat::RGBA16F:      return GL_RGBA16F;
			case FramebufferTextureFormat::RGBA32U:      return GL_RGBA;
			case FramebufferTextureFormat::RED16F:       return GL_R16F;
			case FramebufferTextureFormat::RED_INTEGER:  return GL_RED_INTEGER;
			case FramebufferTextureFormat::RED_FLOAT:    return GL_RED;
		}

		Log::Write(Log::Levels::Error, Log::Categories::Renderer, "Unsupported FramebufferTextureFormat");
		return 0;
	}

}

void OpenGLFramebuffer::CopyFramebufferImpl(unsigned int src_id, unsigned int dest_id, int width, int height)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src_id);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest_id);

	glBlitFramebuffer(
		0, 0, width, height,
		0, 0, width, height,
		GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
		GL_NEAREST
	);

	glBindFramebuffer(GL_FRAMEBUFFER, dest_id);
}

void OpenGLFramebuffer::CopyDepthFramebufferImpl(unsigned int src_id, unsigned int dest_id, int width, int height)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src_id);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest_id);

	glBlitFramebuffer(
		0, 0, width, height,
		0, 0, width, height,
		GL_DEPTH_BUFFER_BIT,
		GL_NEAREST
	);

	glBindFramebuffer(GL_FRAMEBUFFER, dest_id);
}

void OpenGLFramebuffer::CopyColorFramebufferImpl(unsigned int src_id, unsigned int dest_id, int width, int height)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src_id);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest_id);

	glBlitFramebuffer(
		0, 0, width, height,
		0, 0, width, height,
		GL_COLOR_BUFFER_BIT,
		GL_NEAREST
	);

	glBindFramebuffer(GL_FRAMEBUFFER, dest_id);
}

void OpenGLFramebuffer::CopyColorAttachmentImpl(unsigned int src_id, unsigned int dest_id, int width, int height, int src_attach_id, int dest_attach_id)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src_id);
	glReadBuffer(src_attach_id);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest_id);
	glDrawBuffer(dest_attach_id);

	glBlitFramebuffer(
		0, 0, width, height, // Source dimensions
		0, 0, width, height, // Destination dimensions
		GL_COLOR_BUFFER_BIT, // Buffers to copy
		GL_NEAREST // Filter mode
	);

	glBindFramebuffer(GL_FRAMEBUFFER, dest_id);
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
					AttachColorTexture(m_color_attachments[i], m_specification.samples, m_specification.layers, GL_RGBA8, GL_RGBA, GL_FLOAT, m_specification.width, m_specification.height, i);
					break;
				case FramebufferTextureFormat::RGBA16F:
					AttachColorTexture(m_color_attachments[i], m_specification.samples, m_specification.layers, GL_RGBA16F, GL_RGBA, GL_FLOAT, m_specification.width, m_specification.height, i);
					break;
				case FramebufferTextureFormat::RGBA32U:
					AttachColorTexture(m_color_attachments[i], m_specification.samples, m_specification.layers, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, m_specification.width, m_specification.height, i);
					break;
				case FramebufferTextureFormat::RED_INTEGER:
					AttachColorTexture(m_color_attachments[i], m_specification.samples, m_specification.layers, GL_R32I, GL_RED_INTEGER, GL_UNSIGNED_BYTE, m_specification.width, m_specification.height, i);
					break;
				case FramebufferTextureFormat::RED16F:
					AttachColorTexture(m_color_attachments[i], m_specification.samples, m_specification.layers, GL_R16F, GL_RED, GL_FLOAT, m_specification.width, m_specification.height, i);
					break;
				case FramebufferTextureFormat::RED_FLOAT:
					AttachColorTexture(m_color_attachments[i], m_specification.samples, m_specification.layers, GL_RED, GL_RED, GL_FLOAT, m_specification.width, m_specification.height, i);
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
				AttachDepthTexture(m_depth_attachment, m_specification.samples, m_specification.layers, GetMainFramebufferFormat(m_rendererID), GL_DEPTH_ATTACHMENT, m_specification.width, m_specification.height);
				break;
		}
	}

	if (m_color_attachments.size() > 1)
	{
		GLint max_draw_buffers;
		glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);

		DL_ASSERT(m_color_attachments.size() <= max_draw_buffers, Log::Categories::Renderer, "Maximum number of framebuffer attachments exceeded");

		std::vector<GLenum> buffers;
		buffers.reserve(m_color_attachments.size());

		for (auto i = 0; i < m_color_attachments.size(); ++i)
		{
			buffers.push_back(GL_COLOR_ATTACHMENT0 + i);
		}

		glDrawBuffers(m_color_attachments.size(), buffers.data());
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

void OpenGLFramebuffer::BindDrawColorAttachment(int attachment_num)
{
	GLenum single_buffer[] = { GL_COLOR_ATTACHMENT0 + attachment_num };

	glDrawBuffers(1, single_buffer);
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