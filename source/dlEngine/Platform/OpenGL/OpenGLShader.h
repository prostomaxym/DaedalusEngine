#pragma once

#include "Renderer/Resourses/Shader.h"

#include <string>

#include <glad/glad.h>

namespace Daedalus {

	class OpenGLShader final : public Shader
	{
	public:
		OpenGLShader();
		virtual ~OpenGLShader();

		void CreateFromCode(const std::string& code);
		void CreateFromFile(const std::filesystem::path& file_path);
		void CreateFromBinary(const std::filesystem::path& binary_path);
		void SaveBinary(const std::filesystem::path& name) const override;

		OpenGLShader(const OpenGLShader&) = delete;
		OpenGLShader(OpenGLShader&&) = delete;
		OpenGLShader& operator=(const OpenGLShader&) = delete;
		OpenGLShader& operator=(OpenGLShader&&) = delete;

		void Bind() const override;
		void Unbind() const override;

		void SetInt(const std::string& name, int value) const override;
		void SetIntArray(const std::string& name, int* values, uint32_t count) const override;
		void SetFloat(const std::string& name, float value) const override;
		void SetFloat2(const std::string& name, const glm::vec2& value) const override;
		void SetFloat3(const std::string& name, const glm::vec3& value) const override;
		void SetFloat4(const std::string& name, const glm::vec4& value) const override;
		void SetMat3(const std::string& name, const glm::mat3& value) const override;
		void SetMat4(const std::string& name, const glm::mat4& value) const override;

		const std::string& GetName() const override { return m_name; }
		void SetName(const std::string& name) override { m_name = name; }

		void DispatchCompute(uint32_t w, uint32_t h, int local_size) const override;

	private:
		uint32_t m_rendererID;
		std::string m_name;
	};
}