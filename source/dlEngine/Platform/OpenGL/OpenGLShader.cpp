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