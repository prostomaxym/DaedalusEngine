#include "OpenGLShader.h"

#include "OpenGLShaderCompiler.h"

#include <glm/gtc/type_ptr.hpp>

using namespace Daedalus;

OpenGLShader::OpenGLShader() :
	m_rendererID(0)
{
}

void OpenGLShader::CreateFromCode(const std::string& code)
{
	if (m_rendererID > 0)
		return;

	m_rendererID = OpenGLShaderCompiler::BuildFromCode(code);

	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Shader built from code {}", code);
}

void OpenGLShader::CreateFromFile(const std::filesystem::path& file_path)
{
	if (m_rendererID > 0)
		return;

	m_rendererID = OpenGLShaderCompiler::BuildFromFile(file_path);

	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Shader built from source file {}", file_path.string());
}

void OpenGLShader::CreateFromBinary(const std::filesystem::path& binary_path)
{
	if (m_rendererID > 0)
		return;

	m_rendererID = OpenGLShaderCompiler::BuildFromBinary(binary_path);

	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Shaders built from binary file {}", binary_path.string());
}

namespace
{
	void CopyUniform(GLuint source, GLuint target, GLint source_location, GLint target_location, GLenum type)
	{
		GLfloat floats[16]{};
		GLint integers[4]{};
		GLuint unsigned_integers[4]{};
		switch (type)
		{
		case GL_FLOAT: case GL_FLOAT_VEC2: case GL_FLOAT_VEC3: case GL_FLOAT_VEC4:
		case GL_FLOAT_MAT2: case GL_FLOAT_MAT3: case GL_FLOAT_MAT4:
		case GL_FLOAT_MAT2x3: case GL_FLOAT_MAT2x4: case GL_FLOAT_MAT3x2:
		case GL_FLOAT_MAT3x4: case GL_FLOAT_MAT4x2: case GL_FLOAT_MAT4x3:
			glGetUniformfv(source, source_location, floats);
			break;
		case GL_UNSIGNED_INT: case GL_UNSIGNED_INT_VEC2: case GL_UNSIGNED_INT_VEC3: case GL_UNSIGNED_INT_VEC4:
			glGetUniformuiv(source, source_location, unsigned_integers);
			break;
		case GL_INT: case GL_INT_VEC2: case GL_INT_VEC3: case GL_INT_VEC4:
		case GL_BOOL: case GL_BOOL_VEC2: case GL_BOOL_VEC3: case GL_BOOL_VEC4:
#ifndef DL_PLATFORM_ANDROID
		case GL_SAMPLER_1D: case GL_SAMPLER_1D_SHADOW: case GL_SAMPLER_1D_ARRAY:
#endif
		case GL_SAMPLER_2D: case GL_SAMPLER_3D: case GL_SAMPLER_CUBE:
		case GL_SAMPLER_2D_SHADOW: case GL_SAMPLER_CUBE_SHADOW:
		case GL_SAMPLER_2D_ARRAY: case GL_SAMPLER_2D_ARRAY_SHADOW:
		case GL_INT_SAMPLER_2D: case GL_UNSIGNED_INT_SAMPLER_2D:
		case GL_IMAGE_2D: case GL_INT_IMAGE_2D: case GL_UNSIGNED_INT_IMAGE_2D:
			glGetUniformiv(source, source_location, integers);
			break;
		default: return;
		}
		switch (type)
		{
		case GL_FLOAT: glProgramUniform1fv(target, target_location, 1, floats); break;
		case GL_FLOAT_VEC2: glProgramUniform2fv(target, target_location, 1, floats); break;
		case GL_FLOAT_VEC3: glProgramUniform3fv(target, target_location, 1, floats); break;
		case GL_FLOAT_VEC4: glProgramUniform4fv(target, target_location, 1, floats); break;
		case GL_FLOAT_MAT2: glProgramUniformMatrix2fv(target, target_location, 1, GL_FALSE, floats); break;
		case GL_FLOAT_MAT3: glProgramUniformMatrix3fv(target, target_location, 1, GL_FALSE, floats); break;
		case GL_FLOAT_MAT4: glProgramUniformMatrix4fv(target, target_location, 1, GL_FALSE, floats); break;
		case GL_FLOAT_MAT2x3: glProgramUniformMatrix2x3fv(target, target_location, 1, GL_FALSE, floats); break;
		case GL_FLOAT_MAT2x4: glProgramUniformMatrix2x4fv(target, target_location, 1, GL_FALSE, floats); break;
		case GL_FLOAT_MAT3x2: glProgramUniformMatrix3x2fv(target, target_location, 1, GL_FALSE, floats); break;
		case GL_FLOAT_MAT3x4: glProgramUniformMatrix3x4fv(target, target_location, 1, GL_FALSE, floats); break;
		case GL_FLOAT_MAT4x2: glProgramUniformMatrix4x2fv(target, target_location, 1, GL_FALSE, floats); break;
		case GL_FLOAT_MAT4x3: glProgramUniformMatrix4x3fv(target, target_location, 1, GL_FALSE, floats); break;
		case GL_UNSIGNED_INT: glProgramUniform1uiv(target, target_location, 1, unsigned_integers); break;
		case GL_UNSIGNED_INT_VEC2: glProgramUniform2uiv(target, target_location, 1, unsigned_integers); break;
		case GL_UNSIGNED_INT_VEC3: glProgramUniform3uiv(target, target_location, 1, unsigned_integers); break;
		case GL_UNSIGNED_INT_VEC4: glProgramUniform4uiv(target, target_location, 1, unsigned_integers); break;
		case GL_INT_VEC2: case GL_BOOL_VEC2: glProgramUniform2iv(target, target_location, 1, integers); break;
		case GL_INT_VEC3: case GL_BOOL_VEC3: glProgramUniform3iv(target, target_location, 1, integers); break;
		case GL_INT_VEC4: case GL_BOOL_VEC4: glProgramUniform4iv(target, target_location, 1, integers); break;
		default: glProgramUniform1iv(target, target_location, 1, integers); break;
		}
	}

	void CopyProgramState(GLuint source, GLuint target)
	{
		if (!source) return;
		GLint count = 0, max_length = 0;
		glGetProgramiv(source, GL_ACTIVE_UNIFORMS, &count);
		glGetProgramiv(source, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_length);
		std::string buffer(static_cast<size_t>((std::max)(max_length, 1)), '\0');
		for (GLint index = 0; index < count; ++index)
		{
			GLsizei length = 0;
			GLint size = 0;
			GLenum type = 0;
			glGetActiveUniform(source, static_cast<GLuint>(index), static_cast<GLsizei>(buffer.size()), &length, &size, &type, buffer.data());
			const std::string name(buffer.data(), static_cast<size_t>(length));
			const auto resource = glGetProgramResourceIndex(target, GL_UNIFORM, name.c_str());
			if (resource == GL_INVALID_INDEX) continue;
			const GLenum properties[]{ GL_TYPE, GL_ARRAY_SIZE };
			GLint values[2]{};
			glGetProgramResourceiv(target, GL_UNIFORM, resource, 2, properties, 2, nullptr, values);
			if (static_cast<GLenum>(values[0]) != type) continue;
			for (GLint element = 0; element < (std::min)(size, values[1]); ++element)
			{
				auto element_name = name;
				const auto array = element_name.find("[0]");
				if (array != std::string::npos) element_name.replace(array, 3, "[" + std::to_string(element) + "]");
				const auto old_location = glGetUniformLocation(source, element_name.c_str());
				const auto new_location = glGetUniformLocation(target, element_name.c_str());
				if (old_location >= 0 && new_location >= 0) CopyUniform(source, target, old_location, new_location, type);
			}
		}
		for (const GLenum interface_type : { GL_UNIFORM_BLOCK, GL_SHADER_STORAGE_BLOCK })
		{
#ifdef DL_PLATFORM_ANDROID
			// GLES SSBO binding points are specified by the shader's layout qualifiers.
			if (interface_type == GL_SHADER_STORAGE_BLOCK) continue;
#endif
			glGetProgramInterfaceiv(source, interface_type, GL_ACTIVE_RESOURCES, &count);
			glGetProgramInterfaceiv(source, interface_type, GL_MAX_NAME_LENGTH, &max_length);
			buffer.resize(static_cast<size_t>((std::max)(max_length, 1)));
			for (GLint index = 0; index < count; ++index)
			{
				glGetProgramResourceName(source, interface_type, static_cast<GLuint>(index), static_cast<GLsizei>(buffer.size()), nullptr, buffer.data());
				const auto resource = glGetProgramResourceIndex(target, interface_type, buffer.c_str());
				if (resource == GL_INVALID_INDEX) continue;
				const GLenum property = GL_BUFFER_BINDING;
				GLint binding = 0;
				glGetProgramResourceiv(source, interface_type, static_cast<GLuint>(index), 1, &property, 1, nullptr, &binding);
				if (interface_type == GL_UNIFORM_BLOCK) glUniformBlockBinding(target, resource, static_cast<GLuint>(binding));
#ifndef DL_PLATFORM_ANDROID
				else glShaderStorageBlockBinding(target, resource, static_cast<GLuint>(binding));
#endif
			}
		}
	}
}

bool OpenGLShader::ReloadFromCode(const std::string& code)
{
	const auto replacement = OpenGLShaderCompiler::TryBuildFromCode(code);
	if (!replacement) return false;
	CopyProgramState(m_rendererID, replacement);
	GLint bound = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &bound);
	if (m_rendererID != 0 && static_cast<GLuint>(bound) == m_rendererID) glUseProgram(replacement);
	if (m_rendererID != 0) glDeleteProgram(m_rendererID);
	m_rendererID = replacement;
	return true;
}

OpenGLShader::~OpenGLShader()
{
	if (m_rendererID > 0)
		glDeleteProgram(m_rendererID);
}

void OpenGLShader::SaveBinary(const std::filesystem::path& name) const
{
	OpenGLShaderCompiler::SaveBinary(name, m_rendererID);
}

void OpenGLShader::Bind() const
{
	glUseProgram(m_rendererID);
}

void OpenGLShader::Unbind() const
{
	glUseProgram(0);
}

void OpenGLShader::SetInt(const std::string& name, int value) const
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniform1i(location, value);
}

void OpenGLShader::SetIntArray(const std::string& name, int* values, uint32_t count) const
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniform1iv(location, count, values);
}

void OpenGLShader::SetFloat(const std::string& name, float value) const
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniform1f(location, value);
}

void OpenGLShader::SetFloat2(const std::string& name, const glm::vec2& value) const
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniform2f(location, value.x, value.y);
}

void OpenGLShader::SetFloat3(const std::string& name, const glm::vec3& value) const
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniform3f(location, value.x, value.y, value.z);
}

void OpenGLShader::SetFloat4(const std::string& name, const glm::vec4& value) const
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniform4f(location, value.x, value.y, value.z, value.w);
}

void OpenGLShader::SetMat3(const std::string& name, const glm::mat3& value) const
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& value) const
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

namespace
{
	const char* GLTypeToString(GLenum type)
	{
		switch (type)
		{
		case GL_FLOAT: return "float";
		case GL_FLOAT_VEC2: return "vec2";
		case GL_FLOAT_VEC3: return "vec3";
		case GL_FLOAT_VEC4: return "vec4";
		case GL_INT: return "int";
		case GL_INT_VEC2: return "ivec2";
		case GL_INT_VEC3: return "ivec3";
		case GL_INT_VEC4: return "ivec4";
		case GL_UNSIGNED_INT: return "uint";
		case GL_BOOL: return "bool";
		case GL_FLOAT_MAT3: return "mat3";
		case GL_FLOAT_MAT4: return "mat4";
		case GL_SAMPLER_2D: return "sampler2D";
		case GL_SAMPLER_3D: return "sampler3D";
		case GL_SAMPLER_CUBE: return "samplerCube";
		case GL_SAMPLER_2D_SHADOW: return "sampler2DShadow";
		case GL_IMAGE_2D: return "image2D";
		default: return "other";
		}
	}
}

ShaderInfo OpenGLShader::GetInfo() const
{
	ShaderInfo info;
	info.renderer_id = m_rendererID;

	if (m_rendererID == 0)
		return info;

	GLint value = 0;
	glGetProgramiv(m_rendererID, GL_LINK_STATUS, &value);
	info.linked = value == GL_TRUE;

	glGetProgramiv(m_rendererID, GL_PROGRAM_BINARY_LENGTH, &info.binary_length);
	glGetProgramiv(m_rendererID, GL_ACTIVE_UNIFORM_BLOCKS, &info.uniform_block_count);

	GLint log_length = 0;
	glGetProgramiv(m_rendererID, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length > 1)
	{
		info.info_log.resize(static_cast<size_t>(log_length));
		glGetProgramInfoLog(m_rendererID, log_length, nullptr, info.info_log.data());
		info.info_log.resize(static_cast<size_t>(log_length - 1));
	}

	// Querying work group size on a non-compute program raises GL_INVALID_OPERATION
	while (glGetError() != GL_NO_ERROR) {}
	glGetProgramiv(m_rendererID, GL_COMPUTE_WORK_GROUP_SIZE, info.compute_work_group_size);
	info.is_compute = glGetError() == GL_NO_ERROR;
	if (!info.is_compute)
	{
		info.compute_work_group_size[0] = info.compute_work_group_size[1] = info.compute_work_group_size[2] = 0;
	}

	const auto collect = [this](GLenum count_param, GLenum length_param, bool uniforms, std::vector<ShaderVariableInfo>& out)
		{
			GLint count = 0;
			GLint max_length = 0;
			glGetProgramiv(m_rendererID, count_param, &count);
			glGetProgramiv(m_rendererID, length_param, &max_length);

			std::string buffer(static_cast<size_t>(max_length > 0 ? max_length : 1), '\0');
			for (GLint i = 0; i < count; i++)
			{
				GLsizei length = 0;
				GLint size = 0;
				GLenum type = 0;

				if (uniforms)
					glGetActiveUniform(m_rendererID, i, static_cast<GLsizei>(buffer.size()), &length, &size, &type, buffer.data());
				else
					glGetActiveAttrib(m_rendererID, i, static_cast<GLsizei>(buffer.size()), &length, &size, &type, buffer.data());

				ShaderVariableInfo var;
				var.name.assign(buffer.data(), static_cast<size_t>(length));
				var.type = GLTypeToString(type);
				var.size = size;
				var.location = uniforms ? glGetUniformLocation(m_rendererID, var.name.c_str())
					: glGetAttribLocation(m_rendererID, var.name.c_str());
				out.push_back(std::move(var));
			}
		};

	collect(GL_ACTIVE_UNIFORMS, GL_ACTIVE_UNIFORM_MAX_LENGTH, true, info.uniforms);
	collect(GL_ACTIVE_ATTRIBUTES, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, false, info.attributes);

	return info;
}

void OpenGLShader::DispatchCompute(uint32_t w, uint32_t h, int local_size) const
{
	glDispatchCompute((w + local_size - 1) / local_size, (h + local_size - 1) / local_size, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}