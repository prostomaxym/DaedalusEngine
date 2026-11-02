#pragma once

#include "OpenGLHeaders.h"

namespace Daedalus
{
	class ScopedTextureBinding
	{
	public:
		ScopedTextureBinding(GLenum target, GLuint texture) : m_target(target)
		{
			glGetIntegerv(target == GL_TEXTURE_CUBE_MAP ? GL_TEXTURE_BINDING_CUBE_MAP : GL_TEXTURE_BINDING_2D, &m_previous);
			glBindTexture(target, texture);
		}
		~ScopedTextureBinding() { glBindTexture(m_target, static_cast<GLuint>(m_previous)); }
		ScopedTextureBinding(const ScopedTextureBinding&) = delete;
		ScopedTextureBinding& operator=(const ScopedTextureBinding&) = delete;
	private:
		GLenum m_target;
		GLint m_previous{};
	};

	inline void BindTextureUnit(GLuint slot, GLenum target, GLuint texture)
	{
#ifdef DL_PLATFORM_ANDROID
		GLint active = GL_TEXTURE0;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(target, texture);
		glActiveTexture(active);
#else
		glBindTextureUnit(slot, texture);
#endif
	}
}
