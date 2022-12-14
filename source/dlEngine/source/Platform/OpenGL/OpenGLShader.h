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

		virtual void SetInt(const std::string& name, int value) override;
		virtual void SetIntArray(const std::string& name, int* values, uint32_t count) override;
		virtual void SetFloat(const std::string& name, float value) override;
		virtual void SetFloat2(const std::string& name, const glm::vec2& value) override;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;
		virtual void SetMat3(const std::string& name, const glm::mat3& value) override;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) override;

		virtual const std::string& GetName() const override { return m_name; }
		virtual void SetName(const std::string& name) override { m_name = name; }

	private:
		std::pair<GLuint, GLuint> Compile(const GLchar* vert_source, const GLchar* frag_source) const;
		void Link(GLuint vert_shader_id, GLuint frag_shader_id);

		void Pack(const std::string& file_name, const ShaderBinaryData& data) const;
		ShaderBinaryData Unpack(const std::string& file_name) const;

	private:
		uint32_t m_rendererID;
		std::string m_name;
	};
}