#pragma once

#include <string>
#include <string_view>

namespace Daedalus
{
	inline constexpr std::string_view GlesShaderPreamble =
		"#version 320 es\nprecision highp float;\nprecision highp int;\n"
		"precision highp sampler2D;\nprecision highp samplerCube;\n"
		"precision highp sampler2DArray;\nprecision highp image2D;\n";

	[[nodiscard]] inline std::string AdaptShaderDialect(std::string_view source, bool gles)
	{
		if (!gles) return std::string(source);
		std::string result(source);
		const auto version = result.find("#version");
		if (version == std::string::npos) return std::string(GlesShaderPreamble) + result;
		const auto end = result.find('\n', version);
		result.replace(version, end == std::string::npos ? result.size() - version : end - version + 1, GlesShaderPreamble);
		return result;
	}
}
