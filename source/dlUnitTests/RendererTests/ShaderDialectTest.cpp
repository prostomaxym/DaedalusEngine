#include <gtest/gtest.h>
#include "Renderer/Resources/ShaderDialect.h"

namespace Daedalus
{
	TEST(ShaderDialectTest, desktop_source_is_unchanged)
	{
		const std::string source = "#version 450 core\r\nvoid main() {}\r\n";
		EXPECT_EQ(AdaptShaderDialect(source, false), source);
	}

	TEST(ShaderDialectTest, replaces_version_and_adds_es_precision)
	{
		EXPECT_EQ(AdaptShaderDialect("#version 450 core\nvoid main() {}\n", true),
			std::string(GlesShaderPreamble) + "void main() {}\n");
	}

	TEST(ShaderDialectTest, strips_crlf_from_version_directive)
	{
		EXPECT_EQ(AdaptShaderDialect("#version 450\r\nvoid main() {}", true),
			std::string(GlesShaderPreamble) + "void main() {}");
	}

	TEST(ShaderDialectTest, supplies_version_when_absent)
	{
		EXPECT_EQ(AdaptShaderDialect("void main() {}", true), std::string(GlesShaderPreamble) + "void main() {}");
	}

	TEST(ShaderDialectTest, handles_version_without_final_newline)
	{
		EXPECT_EQ(AdaptShaderDialect("#version 450", true), GlesShaderPreamble);
	}
}
