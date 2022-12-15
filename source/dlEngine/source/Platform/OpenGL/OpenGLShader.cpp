#include "OpenGLShader.h"

#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <fstream>

using namespace Daedalus;

OpenGLShader::OpenGLShader(const std::string& vert_code, const std::string& frag_code, InputType input_type) :
	m_rendererID(0)
{
	std::string vert_source = vert_code, frag_source = frag_code;

	if (input_type == InputType::PATH)
	{
		std::ifstream vert_shader_file, frag_shader_file;
		vert_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		frag_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			vert_shader_file.open(vert_code);
			frag_shader_file.open(frag_code);

			std::stringstream vShaderStream, fShaderStream;
			vShaderStream << vert_shader_file.rdbuf();
			fShaderStream << frag_shader_file.rdbuf();

			vert_shader_file.close();
			frag_shader_file.close();

			vert_source = vShaderStream.str();
			frag_source = fShaderStream.str();
		}
		catch (std::ifstream::failure& e)
		{
			DL_CORE_CRITICAL("Failed to open Shader file: " + vert_code + ", " + frag_code);
			return;
		}
	}

	const auto compile_output = Compile((const GLchar*)vert_source.c_str(), (const GLchar*)frag_source.c_str());
	Link(compile_output.first, compile_output.second);

	DL_CORE_INFO("Shaders builed: " + vert_code + ", " + frag_code);
}

OpenGLShader::OpenGLShader(const std::string& compiled_shader_path) :
	m_rendererID(0)
{
	const auto shader_data = Unpack(compiled_shader_path);
	LoadBinary(shader_data);

	DL_CORE_INFO("Prebuild shader loaded: " + compiled_shader_path);
}

OpenGLShader::~OpenGLShader()
{
	glDeleteProgram(m_rendererID);
}

void OpenGLShader::SaveBinary(const std::string& name) const
{
	ShaderBinaryData data;
	glGetProgramiv(m_rendererID, GL_PROGRAM_BINARY_LENGTH, &data.length);

	data.binary.resize(data.length);
	glGetProgramBinary(m_rendererID, data.length, &data.length, &data.format, data.binary.data());

	Pack(name, data);
}

void OpenGLShader::LoadBinary(const ShaderBinaryData& data)
{
	m_rendererID = glCreateProgram();
	glProgramBinary(m_rendererID, data.format, data.binary.data(), data.length);

	GLint isLinked = 0;
	glGetProgramiv(m_rendererID, GL_LINK_STATUS, (int*)&isLinked);
	if (isLinked == GL_FALSE)
	{
		GLint max_length = 0;
		glGetProgramiv(m_rendererID, GL_INFO_LOG_LENGTH, &max_length);
		glDeleteProgram(m_rendererID);

		if (max_length > 0)
		{
			std::vector<GLchar> info_log(max_length);
			glGetProgramInfoLog(m_rendererID, max_length, &max_length, &info_log[0]);
			std::string log_text(info_log.data());
			DL_CORE_CRITICAL("Shader binary loading error message: " + log_text);
			throw std::runtime_error(info_log.data());
		}
		else
		{
			DL_CORE_CRITICAL("Shader binary loading with empty error message;");
			throw std::runtime_error("Shader binary loading with empty error message;");
		}
	}
}

void OpenGLShader::Bind() const
{
	glUseProgram(m_rendererID);
}

void OpenGLShader::Unbind() const
{
	glUseProgram(0);
}

std::pair<GLuint, GLuint> OpenGLShader::Compile(const GLchar* vert_source, const GLchar* frag_source) const
{
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	
	glShaderSource(vertex_shader, 1, &vert_source, 0);
	glCompileShader(vertex_shader);

	GLint is_compiled = 0;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &is_compiled);
	if (is_compiled == GL_FALSE)
	{
		GLint max_length = 0;
		glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &max_length);
		glDeleteProgram(m_rendererID);

		if (max_length > 0)
		{
			std::vector<GLchar> info_log(max_length);
			glGetProgramInfoLog(m_rendererID, max_length, &max_length, &info_log[0]);
			std::string log_text(info_log.data());
			DL_CORE_CRITICAL("Vertex shader compilation error: " + log_text);
			throw std::runtime_error(info_log.data());
		}
		else
		{
			DL_CORE_CRITICAL("Vertex shader compilation with empty error message;");
			throw std::runtime_error("Vertex shader compilation with empty error message;");
		}
	}

	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(fragment_shader, 1, &frag_source, 0);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &is_compiled);

	if (is_compiled == GL_FALSE)
	{
		GLint max_length = 0;
		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &max_length);
		glDeleteProgram(m_rendererID);

		if (max_length > 0)
		{
			std::vector<GLchar> info_log(max_length);
			glGetProgramInfoLog(m_rendererID, max_length, &max_length, &info_log[0]);
			std::string log_text(info_log.data());
			DL_CORE_CRITICAL("Fragment shader compilation error: " + log_text);
			throw std::runtime_error(info_log.data());
		}
		else
		{
			throw std::runtime_error("Fragment shader compilation with empty error message;");
		}
	}

	return { vertex_shader, fragment_shader };
}

void OpenGLShader::Link(GLuint vert_shader_id, GLuint frag_shader_id)
{
	m_rendererID = glCreateProgram();

	glAttachShader(m_rendererID, vert_shader_id);
	glAttachShader(m_rendererID, frag_shader_id);

	glLinkProgram(m_rendererID);

	GLint isLinked = 0;
	glGetProgramiv(m_rendererID, GL_LINK_STATUS, (int*)&isLinked);
	if (isLinked == GL_FALSE)
	{
		GLint max_length = 0;
		glGetProgramiv(m_rendererID, GL_INFO_LOG_LENGTH, &max_length);
		glDeleteProgram(m_rendererID);

		if (max_length > 0)
		{
			std::vector<GLchar> info_log(max_length);
			glGetProgramInfoLog(m_rendererID, max_length, &max_length, &info_log[0]);
			std::string log_text(info_log.data());
			DL_CORE_CRITICAL("Shader linking error: " + log_text);
			throw std::runtime_error(info_log.data());
		}
		else
		{
			throw std::runtime_error("Shader linking error with empty error message;");
		}
	}

	glDetachShader(m_rendererID, vert_shader_id);
	glDetachShader(m_rendererID, frag_shader_id);
}

void OpenGLShader::Pack(const std::string& file_name, const ShaderBinaryData& data) const
{
	try
	{
		std::ofstream bin_file(file_name, std::ios::out | std::ios::binary);
		bin_file.write(reinterpret_cast<const char*>(&data.format), sizeof(data.format));
		bin_file.write(reinterpret_cast<const char*>(&data.length), sizeof(data.length));
		bin_file.write(reinterpret_cast<const char*>(data.binary.data()), data.length);
	}
	catch (std::ifstream::failure& e)
	{
		DL_CORE_CRITICAL("Failed to pack shader: " + file_name);
		throw std::runtime_error("Failed to pack shader data");
	}
}

Shader::ShaderBinaryData OpenGLShader::Unpack(const std::string& file_name) const
{
	ShaderBinaryData data;

	try
	{
		std::ifstream bin_file(file_name, std::ios::in | std::ios::binary);
		bin_file.read(reinterpret_cast<char*>(&data.format), sizeof(data.format));
		bin_file.read(reinterpret_cast<char*>(&data.length), sizeof(data.length));
		data.binary.resize(data.length);
		bin_file.read(reinterpret_cast<char*>(data.binary.data()), data.length);
	}
	catch (std::ifstream::failure& e)
	{
		DL_CORE_CRITICAL("Failed to unpack shader: " + file_name);
		throw std::runtime_error("Failed to unpack shader data");
	}

	return data;
}

void OpenGLShader::SetInt(const std::string& name, int value)
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniform1i(location, value);
}

void OpenGLShader::SetIntArray(const std::string& name, int* values, uint32_t count)
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniform1iv(location, count, values);
}

void OpenGLShader::SetFloat(const std::string& name, float value)
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniform1f(location, value);
}

void OpenGLShader::SetFloat2(const std::string& name, const glm::vec2& value)
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniform2f(location, value.x, value.y);
}

void OpenGLShader::SetFloat3(const std::string& name, const glm::vec3& value)
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniform3f(location, value.x, value.y, value.z);
}

void OpenGLShader::SetFloat4(const std::string& name, const glm::vec4& value)
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniform4f(location, value.x, value.y, value.z, value.w);
}

void OpenGLShader::SetMat3(const std::string& name, const glm::mat3& value)
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& value)
{
	GLint location = glGetUniformLocation(m_rendererID, name.c_str());
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}