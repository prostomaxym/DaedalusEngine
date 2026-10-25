#include <Debug/Log.h>
#include <gtest/gtest.h>

int main(int argc, char** argv) 
{
	::testing::InitGoogleTest(&argc, argv);
	Daedalus::Log::Init();
	const auto result = RUN_ALL_TESTS();
	Daedalus::Log::Shutdown();
	return result;
}