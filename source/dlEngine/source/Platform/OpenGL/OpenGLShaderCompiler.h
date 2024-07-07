#pragma once

#include "Daedalus/Renderer/Resourses/Shader.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <glad/glad.h>

namespace Daedalus {

	class OpenGLShaderCompiler final
	{
	public:
		enum class ShaderType
		{
			Vertex,
			Fragment,
			Geometry,
			Tessellation,
			Compute,
			Unknown
		};

	public:
		static GLuint BuildFromFile(const std::filesystem::path& source_path);
		static GLuint BuildFromBinary(const std::filesystem::path& binary_path);
		static GLuint BuildFromCode(const std::string& code);

		static void SaveBinary(const std::filesystem::path& file_name, GLuint shader_id);

	private:
		static GLuint CompileFromCode(const std::string& code, ShaderType type);
		static std::vector<GLuint> CompileMultipleShaders(const std::map<ShaderType, std::string>& shaders);
		static GLuint Link(const std::vector<GLuint>& shaders);

		static std::map<ShaderType, std::string> SplitFileIntoShaders(const std::filesystem::path& file_path);
		static std::map<ShaderType, std::string> SplitCodeIntoShaders(const std::string& content);

		static GLenum GetShaderGLType(ShaderType type);
		static std::string GetShaderTypeString(ShaderType type);

		static void Pack(const std::filesystem::path& file_name, const Shader::ShaderBinaryData& data);
		static Shader::ShaderBinaryData Unpack(const std::filesystem::path& file_name);
	};
}