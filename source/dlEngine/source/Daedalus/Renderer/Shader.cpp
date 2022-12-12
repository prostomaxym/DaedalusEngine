#include "Shader.h"

#include <vector>
#include <fstream>

using namespace Daedalus;

Shader::Shader(const std::string& vert_code, const std::string& frag_code, InputType input_type) :
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
			// LOG
		}
	}

	const auto compile_output = Compile((const GLchar*)vert_source.c_str(), (const GLchar*)frag_source.c_str());
	Link(compile_output.first, compile_output.second);
}

Shader::Shader(const std::string& compiled_shader_path) :
	m_rendererID(0)
{
	const auto shader_data = Unpack(compiled_shader_path);
	LoadBinary(shader_data);
}

Shader::~Shader()
{
	glDeleteProgram(m_rendererID);
}

void Shader::SaveBinary(const std::string& name) const
{
	ShaderBinaryData data;
	glGetProgramiv(m_rendererID, GL_PROGRAM_BINARY_LENGTH, &data.length);

	data.binary.resize(data.length);
	glGetProgramBinary(m_rendererID, data.length, &data.length, &data.format, data.binary.data());

	Pack(name, data);
}

void Shader::LoadBinary(const ShaderBinaryData& data)
{
	m_rendererID = glCreateProgram();
	glProgramBinary(m_rendererID, data.format, data.binary.data(), data.length);

	GLint isLinked = 0;
	glGetProgramiv(m_rendererID, GL_LINK_STATUS, (int*)&isLinked);
	if (isLinked == GL_FALSE)
	{
		GLint max_length = 0;
		glGetProgramiv(m_rendererID, GL_INFO_LOG_LENGTH, &max_length);

		std::vector<GLchar> info_log(max_length);
		glGetProgramInfoLog(m_rendererID, max_length, &max_length, &info_log[0]);

		glDeleteProgram(m_rendererID);

		throw std::runtime_error(info_log.data());
	}
}

void Shader::Bind() const
{
	glUseProgram(m_rendererID);
}

void Shader::Unbind() const
{
	glUseProgram(0);
}

std::pair<GLuint, GLuint> Shader::Compile(const GLchar* vert_source, const GLchar* frag_source) const
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

		std::vector<GLchar> info_log(max_length);
		glGetShaderInfoLog(vertex_shader, max_length, &max_length, &info_log[0]);
		glDeleteShader(vertex_shader);

		throw std::runtime_error(info_log.data());
	}

	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(fragment_shader, 1, &frag_source, 0);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &is_compiled);

	if (is_compiled == GL_FALSE)
	{
		GLint max_length = 0;
		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &max_length);

		std::vector<GLchar> info_log(max_length);
		glGetShaderInfoLog(fragment_shader, max_length, &max_length, &info_log[0]);

		glDeleteShader(fragment_shader);
		glDeleteShader(vertex_shader);

		throw std::runtime_error(info_log.data());
	}

	return { vertex_shader, fragment_shader };
}

void Shader::Link(GLuint vert_shader_id, GLuint frag_shader_id)
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

		std::vector<GLchar> info_log(max_length);
		glGetProgramInfoLog(m_rendererID, max_length, &max_length, &info_log[0]);

		glDeleteProgram(m_rendererID);
		glDeleteShader(vert_shader_id);
		glDeleteShader(frag_shader_id);

		throw std::runtime_error(info_log.data());
	}

	glDetachShader(m_rendererID, vert_shader_id);
	glDetachShader(m_rendererID, frag_shader_id);
}

void Shader::Pack(const std::string& file_name, const ShaderBinaryData& data) const
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
		throw std::runtime_error("Failed to pack shader data");
	}
}

Shader::ShaderBinaryData Shader::Unpack(const std::string& file_name) const
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
		throw std::runtime_error("Failed to unpack shader data");
	}

	return data;
}
