#pragma once

#include <string>

#include <glad/glad.h>

namespace Daedalus {

	class Shader
	{
	private:
		struct ShaderBinaryData
		{
			GLenum format = 0;
			GLsizei length = 0;
			std::vector<GLubyte> binary;
		};

	public:
		enum class InputType
		{
			PATH,
			SOURCE
		};

		Shader(const std::string& vert_code, const std::string& frag_code, InputType input_type = InputType::PATH);
		Shader(const std::string& compiled_shader_path);
		~Shader();

		Shader(const Shader&) = delete;
		Shader(Shader&&) = delete;
		Shader& operator=(const Shader&) = delete;
		Shader& operator=(Shader&&) = delete;

		void SaveBinary(const std::string& name) const;
		void LoadBinary(const ShaderBinaryData& data);

		void Bind() const;
		void Unbind() const;

	private:
		std::pair<GLuint, GLuint> Compile(const GLchar* vert_source, const GLchar* frag_source) const;
		void Link(GLuint vert_shader_id, GLuint frag_shader_id);

		void Pack(const std::string& file_name, const ShaderBinaryData& data) const;
		ShaderBinaryData Unpack(const std::string& file_name) const;

		uint32_t m_rendererID;
	};
}