#pragma once

#include <string>
#include <memory>

namespace Daedalus {

	class Shader
	{
	public:
		enum class InputType
		{
			PATH,
			SOURCE,
			BINARY
		};
		struct ShaderBinaryData
		{
			unsigned int format = 0;
			int length = 0;
			std::vector<unsigned char> binary;
		};

	public:
		static std::shared_ptr<Shader> Create(const std::string& compiled_shader_path);
		static std::shared_ptr<Shader> Create(const std::string& vert_code, const std::string& frag_code, InputType input_type = InputType::PATH);

		virtual ~Shader() = default;

		virtual void SaveBinary(const std::string& name) const = 0;
		virtual void LoadBinary(const ShaderBinaryData& data) = 0;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;
	};
}