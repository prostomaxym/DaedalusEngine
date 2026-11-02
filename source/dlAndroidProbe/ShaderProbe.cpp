#include "ShaderProbe.h"

#include <GLES3/gl32.h>
#include <android/log.h>
#include <array>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

namespace Daedalus
{
	namespace
	{
		constexpr const char* LogTag = "DaedalusProbe";

		bool ReadSource(const std::filesystem::path& path, std::string& source, int depth = 0)
		{
			if (depth > 16) return false;
			std::ifstream input(path);
			if (!input) return false;
			std::string line;
			while (std::getline(input, line))
			{
				if (!line.empty() && line.back() == '\r') line.pop_back();
				if (line.starts_with("#include \""))
				{
					const auto end = line.find('"', 10);
					if (end == std::string::npos || !ReadSource(path.parent_path() / line.substr(10, end - 10), source, depth + 1)) return false;
				}
				else source += line + '\n';
			}
			return !input.bad();
		}

		bool CheckShaderFile(const std::filesystem::path& path)
		{
			std::string source;
			if (!ReadSource(path, source))
			{
				__android_log_print(ANDROID_LOG_ERROR, LogTag, "Cannot read %s", path.c_str());
				return false;
			}
			std::map<GLenum, std::string> stages;
			GLenum stage = 0;
			std::istringstream input(source);
			std::string line;
			while (std::getline(input, line))
			{
				if (line == "#vertex") stage = GL_VERTEX_SHADER;
				else if (line == "#fragment") stage = GL_FRAGMENT_SHADER;
				else if (line == "#geometry") stage = GL_GEOMETRY_SHADER;
				else if (line == "#compute") stage = GL_COMPUTE_SHADER;
				else if (stage)
				{
					if (line.starts_with("#version"))
					{
						// Probe the proposed GLES dialect without changing production shaders.
						stages[stage] += "#version 320 es\nprecision highp float;\nprecision highp int;\nprecision highp sampler2D;\nprecision highp samplerCube;\nprecision highp sampler2DArray;\nprecision highp image2D;\n";
					}
					else stages[stage] += line + '\n';
				}
			}
			const auto program = glCreateProgram();
			bool passed = !stages.empty();
			for (const auto& [type, text] : stages)
			{
				const auto shader = glCreateShader(type);
				const auto* shader_source = text.c_str();
				glShaderSource(shader, 1, &shader_source, nullptr);
				glCompileShader(shader);
				GLint compiled = GL_FALSE;
				glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
				if (!compiled)
				{
					std::array<char, 8192> error{};
					glGetShaderInfoLog(shader, static_cast<GLsizei>(error.size()), nullptr, error.data());
					__android_log_print(ANDROID_LOG_ERROR, LogTag, "%s stage 0x%x: %s", path.filename().c_str(), type, error.data());
					passed = false;
				}
				glAttachShader(program, shader);
				glDeleteShader(shader);
			}
			glLinkProgram(program);
			GLint linked = GL_FALSE;
			glGetProgramiv(program, GL_LINK_STATUS, &linked);
			if (!linked)
			{
				std::array<char, 8192> error{};
				glGetProgramInfoLog(program, static_cast<GLsizei>(error.size()), nullptr, error.data());
				__android_log_print(ANDROID_LOG_ERROR, LogTag, "%s link: %s", path.filename().c_str(), error.data());
			}
			glDeleteProgram(program);
			passed = linked && passed;
			__android_log_print(passed ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, LogTag, "%s: %s", path.filename().c_str(), passed ? "PASS" : "FAIL");
			return passed;
		}
	}

	bool CheckEngineShaders(const std::filesystem::path& directory)
	{
		constexpr std::array shader_files{
			"Blur.glsl", "Cubemap.glsl", "DeferredGeometryPass.glsl", "DeferredLightPass.glsl",
			"DebugDraw.glsl", "ForwardPass.glsl", "ShadowPass.glsl", "SSAO.glsl",
			"TextureArrayToGrayScale.glsl", "TextureToGrayScale.glsl", "DepthToGrayScale.glsl"
		};
		bool passed = true;
		for (const auto* name : shader_files) passed = CheckShaderFile(directory / name) && passed;
		return passed;
	}
}
