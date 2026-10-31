#include <Debug/BuildInfo.h>

#include <gtest/gtest.h>

#include <string>

using Daedalus::GetBuildInfo;

TEST(BuildInfoTest, reports_application_version)
{
	EXPECT_EQ(GetBuildInfo().find("Application version: " DL_TEST_APPLICATION_VERSION "\n"), 0u);
}

TEST(BuildInfoTest, reports_build_version_from_version_file)
{
	EXPECT_NE(GetBuildInfo().find("\nBuild version: " DL_TEST_APPLICATION_VERSION "\n"), std::string::npos);
}

TEST(BuildInfoTest, reports_configured_build_metadata)
{
	const auto info = GetBuildInfo();

	EXPECT_NE(info.find("\nConfiguration: " DL_TEST_BUILD_CONFIGURATION "\n"), std::string::npos);
	EXPECT_NE(info.find("\nPlatform: " DL_TEST_BUILD_PLATFORM "\n"), std::string::npos);
	EXPECT_NE(info.find("\nCompiler: " DL_TEST_BUILD_COMPILER "\n"), std::string::npos);
}

TEST(BuildInfoTest, reports_target_architecture)
{
#if defined(_M_ARM64EC)
	constexpr auto expected = "\nArchitecture: arm64ec\n";
#elif defined(_M_ARM64) || defined(__aarch64__)
	constexpr auto expected = "\nArchitecture: arm64\n";
#elif defined(_M_X64) || defined(__x86_64__)
	constexpr auto expected = "\nArchitecture: x64\n";
#elif defined(_M_IX86) || defined(__i386__)
	constexpr auto expected = "\nArchitecture: x86\n";
#elif defined(_M_ARM) || defined(__arm__)
	constexpr auto expected = "\nArchitecture: arm\n";
#else
	constexpr auto expected = "\nArchitecture: unknown\n";
#endif

	EXPECT_NE(GetBuildInfo().find(expected), std::string::npos);
}

TEST(BuildInfoTest, reports_developer_tools_status)
{
	const auto expected = Daedalus::IsDevBuild() ? "\nDeveloper tools: enabled" : "\nDeveloper tools: disabled";

	EXPECT_NE(GetBuildInfo().find(expected), std::string::npos);
}

TEST(BuildInfoTest, report_is_stable_between_calls)
{
	EXPECT_EQ(GetBuildInfo(), GetBuildInfo());
}
