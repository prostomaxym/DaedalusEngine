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

	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Shader built from code " + code);
}

void OpenGLShader::CreateFromFile(const std::filesystem::path& file_path)
{
	if (m_rendererID > 0)
		return;

	m_rendererID = OpenGLShaderCompiler::BuildFromFile(file_path);

	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Shader built from source file " + file_path.string());
}

void OpenGLShader::CreateFromBinary(const std::filesystem::path& binary_path)
{
	if (m_rendererID > 0)
		return;

	m_rendererID = OpenGLShaderCompiler::BuildFromBinary(binary_path);

	Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Shaders built from binary file " + binary_path.string());
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