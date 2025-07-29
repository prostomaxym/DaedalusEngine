#pragma once

#include <glm/glm.hpp>

#include <string>
#include <memory>

namespace Daedalus {

	class DAEDALUS_API Shader
	{
	public:
		struct ShaderBinaryData
		{
			unsigned int format = 0;
			int length = 0;
			std::vector<unsigned char> binary;
		};
	public:
		static std::shared_ptr<Shader> CreateFromCode(const std::string& code);
		static std::shared_ptr<Shader> CreateFromFile(const std::filesystem::path& file_path);
		static std::shared_ptr<Shader> CreateFromBinary(const std::filesystem::path& binary_path);

		static std::string GetSourceFileExtention();
		static std::string GetBinaryFileExtention();

		virtual ~Shader() = default;

		virtual void SaveBinary(const std::filesystem::path& name) const = 0;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual const std::string& GetName() const = 0;
		virtual void SetName(const std::string& name) = 0;

		virtual void SetInt(const std::string& name, int value) const = 0;
		virtual void SetIntArray(const std::string& name, int* values, uint32_t count) const = 0;
		virtual void SetFloat(const std::string& name, float value) const = 0;
		virtual void SetFloat2(const std::string& name, const glm::vec2& value) const = 0;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) const = 0;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) const = 0;
		virtual void SetMat3(const std::string& name, const glm::mat3& value) const = 0;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) const = 0;

		virtual void DispatchCompute(uint32_t w, uint32_t h, int local_size) const = 0;
	};
}