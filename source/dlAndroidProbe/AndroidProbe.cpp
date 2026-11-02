#include "ShaderProbe.h"

#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <android/log.h>

#include <array>

namespace Daedalus
{
	namespace
	{
		constexpr const char* LogTag = "DaedalusProbe";

		GLuint Compile(GLenum stage, const char* source)
		{
			const GLuint shader = glCreateShader(stage);
			glShaderSource(shader, 1, &source, nullptr);
			glCompileShader(shader);
			GLint compiled = GL_FALSE;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
			if (!compiled)
			{
				std::array<char, 4096> error{};
				glGetShaderInfoLog(shader, static_cast<GLsizei>(error.size()), nullptr, error.data());
				__android_log_print(ANDROID_LOG_ERROR, LogTag, "Shader %u: %s", stage, error.data());
				glDeleteShader(shader);
				return 0;
			}
			return shader;
		}

		bool CheckProgram(const std::array<GLuint, 3>& shaders)
		{
			const GLuint program = glCreateProgram();
			bool valid = true;
			for (const auto shader : shaders)
			{
				if (shader) glAttachShader(program, shader);
				else valid = false;
			}
			glLinkProgram(program);
			GLint linked = GL_FALSE;
			glGetProgramiv(program, GL_LINK_STATUS, &linked);
			if (!linked)
			{
				std::array<char, 4096> error{};
				glGetProgramInfoLog(program, static_cast<GLsizei>(error.size()), nullptr, error.data());
				__android_log_print(ANDROID_LOG_ERROR, LogTag, "Program: %s", error.data());
			}
			for (const auto shader : shaders) if (shader) glDeleteShader(shader);
			glDeleteProgram(program);
			return valid && linked;
		}

		bool CheckColorFormat(GLenum format)
		{
			GLuint framebuffer = 0, texture = 0;
			glGenFramebuffers(1, &framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexStorage2D(GL_TEXTURE_2D, 1, format, 16, 16);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
			const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			const auto error = glGetError();
			__android_log_print(ANDROID_LOG_INFO, LogTag, "Color format 0x%x: status=0x%x error=0x%x", format, status, error);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &framebuffer);
			glDeleteTextures(1, &texture);
			return status == GL_FRAMEBUFFER_COMPLETE && error == GL_NO_ERROR;
		}

		bool CheckGBuffer()
		{
			constexpr std::array<GLenum, 6> formats{ GL_RGBA16F, GL_RGBA16F, GL_RGBA8, GL_RGBA8, GL_RGBA8, GL_R16F };
			std::array<GLuint, 7> textures{};
			std::array<GLenum, 6> draw_buffers{};
			GLuint framebuffer = 0;
			glGenFramebuffers(1, &framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glGenTextures(static_cast<GLsizei>(textures.size()), textures.data());
			for (size_t i = 0; i < formats.size(); ++i)
			{
				glBindTexture(GL_TEXTURE_2D, textures[i]);
				glTexStorage2D(GL_TEXTURE_2D, 1, formats[i], 16, 16);
				draw_buffers[i] = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
				glFramebufferTexture2D(GL_FRAMEBUFFER, draw_buffers[i], GL_TEXTURE_2D, textures[i], 0);
			}
			glBindTexture(GL_TEXTURE_2D, textures.back());
			glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, 16, 16);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textures.back(), 0);
			glDrawBuffers(static_cast<GLsizei>(draw_buffers.size()), draw_buffers.data());
			const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			const auto error = glGetError();
			__android_log_print(ANDROID_LOG_INFO, LogTag, "Six-attachment G-buffer: status=0x%x error=0x%x", status, error);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &framebuffer);
			glDeleteTextures(static_cast<GLsizei>(textures.size()), textures.data());
			return status == GL_FRAMEBUFFER_COMPLETE && error == GL_NO_ERROR;
		}

		bool CheckLayeredDepth()
		{
			GLuint framebuffer = 0, texture = 0;
			glGenFramebuffers(1, &framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
			glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT32F, 16, 16, 32);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			constexpr std::array<GLfloat, 4> border{ 1.f, 1.f, 1.f, 1.f };
			glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border.data());
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture, 0);
			constexpr GLenum draw_buffer = GL_NONE;
			glDrawBuffers(1, &draw_buffer);
			glReadBuffer(GL_NONE);
			const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			const auto error = glGetError();
			__android_log_print(ANDROID_LOG_INFO, LogTag, "Layered depth/border clamp: status=0x%x error=0x%x", status, error);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &framebuffer);
			glDeleteTextures(1, &texture);
			return status == GL_FRAMEBUFFER_COMPLETE && error == GL_NO_ERROR;
		}
	}

	int RunAndroidProbe(const char* shader_directory)
	{
		const auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		if (display == EGL_NO_DISPLAY || !eglInitialize(display, nullptr, nullptr))
		{
			__android_log_print(ANDROID_LOG_ERROR, LogTag, "eglInitialize failed: 0x%x", eglGetError());
			return 1;
		}
		const EGLint config_attributes[] = { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_NONE };
		EGLConfig config = nullptr;
		EGLint count = 0;
		eglChooseConfig(display, config_attributes, &config, 1, &count);
		if (!count) { eglTerminate(display); return 1; }
		const EGLint context_attributes[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 2, EGL_NONE };
		const auto context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
		const EGLint surface_attributes[] = { EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE };
		const auto surface = eglCreatePbufferSurface(display, config, surface_attributes);
		if (context == EGL_NO_CONTEXT || surface == EGL_NO_SURFACE || !eglMakeCurrent(display, surface, surface, context))
		{
			__android_log_print(ANDROID_LOG_ERROR, LogTag, "EGL context/surface failed: 0x%x", eglGetError());
			if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
			if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
			eglTerminate(display);
			return 1;
		}
		__android_log_print(ANDROID_LOG_INFO, LogTag, "%s | %s", glGetString(GL_RENDERER), glGetString(GL_VERSION));
		bool passed = true;
		struct Limit { GLenum name; const char* label; GLint required; };
		constexpr std::array limits{
			Limit{ GL_MAX_GEOMETRY_SHADER_INVOCATIONS, "geometry invocations", 32 },
			Limit{ GL_MAX_GEOMETRY_OUTPUT_VERTICES, "geometry output vertices", 3 },
			Limit{ GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, "fragment SSBOs", 2 },
			Limit{ GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS, "geometry SSBOs", 2 },
			Limit{ GL_MAX_DRAW_BUFFERS, "draw buffers", 6 },
			Limit{ GL_MAX_COLOR_ATTACHMENTS, "color attachments", 6 },
			Limit{ GL_MAX_ARRAY_TEXTURE_LAYERS, "array layers", 32 },
			Limit{ GL_MAX_TEXTURE_SIZE, "texture size", 2048 }
		};
		for (const auto& limit : limits)
		{
			GLint value = 0;
			glGetIntegerv(limit.name, &value);
			__android_log_print(ANDROID_LOG_INFO, LogTag, "%s: %d (required %d)", limit.label, value, limit.required);
			passed = value >= limit.required && passed;
		}
		const char* vertex = "#version 320 es\nlayout(location=0) in vec3 p; void main(){gl_Position=vec4(p,1.0);}";
		const char* geometry = "#version 320 es\nprecision highp float; layout(triangles,invocations=32) in; layout(triangle_strip,max_vertices=3) out; layout(std430,binding=0) readonly buffer Matrices { mat4 transforms[]; }; void main(){ for(int i=0;i<3;++i){ gl_Layer=gl_InvocationID; gl_Position=transforms[gl_InvocationID]*gl_in[i].gl_Position; EmitVertex(); } EndPrimitive(); }";
		const char* fragment = "#version 320 es\nprecision highp float; layout(location=0) out vec4 color; layout(std430,binding=1) readonly buffer Lights { vec4 lights[]; }; void main(){color=lights[0];}";
		passed = CheckProgram({ Compile(GL_VERTEX_SHADER, vertex), Compile(GL_GEOMETRY_SHADER, geometry), Compile(GL_FRAGMENT_SHADER, fragment) }) && passed;
		const char* compute = "#version 320 es\nprecision highp float; layout(local_size_x=16,local_size_y=16) in; layout(rgba8,binding=1) writeonly uniform highp image2D destination; void main(){imageStore(destination,ivec2(gl_GlobalInvocationID.xy),vec4(1.0));}";
		const auto compute_shader = Compile(GL_COMPUTE_SHADER, compute);
		const auto compute_program = glCreateProgram();
		if (compute_shader) glAttachShader(compute_program, compute_shader);
		glLinkProgram(compute_program);
		GLint linked = GL_FALSE;
		glGetProgramiv(compute_program, GL_LINK_STATUS, &linked);
		passed = compute_shader && linked && passed;
		if (compute_shader) glDeleteShader(compute_shader);
		glDeleteProgram(compute_program);
		for (const auto format : { GL_RGBA8, GL_RGBA16F, GL_R16F, GL_R32I }) passed = CheckColorFormat(format) && passed;
		passed = CheckGBuffer() && passed;
		passed = CheckLayeredDepth() && passed;
		if (shader_directory) passed = CheckEngineShaders(shader_directory) && passed;
		passed = glGetError() == GL_NO_ERROR && passed;
		__android_log_print(passed ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, LogTag, "RESULT: %s", passed ? "PASS" : "FAIL");
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroySurface(display, surface);
		eglDestroyContext(display, context);
		eglTerminate(display);
		return passed ? 0 : 1;
	}
}

int main(int argc, char** argv)
{
	return Daedalus::RunAndroidProbe(argc > 1 ? argv[1] : nullptr);
}
