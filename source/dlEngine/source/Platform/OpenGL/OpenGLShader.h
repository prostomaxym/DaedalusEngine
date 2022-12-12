#pragma once

#include "Daedalus/Renderer/Shader.h"

#include <string>

#include <glad/glad.h>

namespace Daedalus {

	class OpenGLShader : public Shader
	{
	public:

		OpenGLShader(const std::string& vert_code, const std::string& frag_code, InputType input_type = InputType::PATH);
		OpenGLShader(const std::string& compiled_shader_path);
		virtual ~OpenGLShader();

		OpenGLShader(const OpenGLShader&) = delete;
		OpenGLShader(OpenGLShader&&) = delete;
		OpenGLShader& operator=(const OpenGLShader&) = delete;
		OpenGLShader& operator=(OpenGLShader&&) = delete;

		void SaveBinary(const std::string& name) const override;
		void LoadBinary(const ShaderBinaryData& data) override;

		void Bind() const override;
		void Unbind() const override;

	private:
		std::pair<GLuint, GLuint> Compile(const GLchar* vert_source, const GLchar* frag_source) const;
		void Link(GLuint vert_shader_id, GLuint frag_shader_id);

		void Pack(const std::string& file_name, const ShaderBinaryData& data) const;
		ShaderBinaryData Unpack(const std::string& file_name) const;

		uint32_t m_rendererID;
	};
}