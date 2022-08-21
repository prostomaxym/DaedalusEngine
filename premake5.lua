workspace "Daedalus"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Production"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}

IncludeDir["GLFW"] = "dlEngine/ThirdParty/GLFW/include"
include "dlEngine/ThirdParty/GLFW/"


---------------------------------------------------------------------------------------------------
project "dlEngine"
	location "dlEngine"
	kind "SharedLib"
	language "C++"
	cppdialect "C++17"

	targetdir("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "dlpch.h"
	pchsource "dlEngine/source/dlpch.cpp"

	files
	{
		"%{prj.name}/source/**.h",
		"%{prj.name}/source/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/source",
		"%{prj.name}/ThirdParty/spdlog/include",
		"%{IncludeDir.GLFW}"
	}

	links 
	{ 
		"GLFW",
		"opengl32.lib"	
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"DL_PLATFORM_WINDOWS",
			"DL_BUILD_DLL"
		}
	
		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/dlSandbox"),
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/dlUnitTests")
		}

	filter "configurations:Debug"
		defines "DL_DEBUG"
		defines "DL_ENABLE_ASSERTS"
		symbols "On"

	filter "configurations:Release"
		defines "DL_RELEASE"
		optimize "On"

	filter "configurations:Production"
		defines "DL_PRODUCTION"
		optimize "On"
---------------------------------------------------------------------------------------------------

project "dlSandbox"
	location "dlSandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"

	targetdir("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/source/**.h",
		"%{prj.name}/source/**.cpp"
	}

	includedirs
	{
		"dlEngine/ThirdParty/spdlog/include",
		"dlEngine/source"
	}

	links
	{
		"dlEngine"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"DL_PLATFORM_WINDOWS",
		}

	filter "configurations:Debug"
		defines "DL_DEBUG"
		defines "DL_ENABLE_ASSERTS"
		symbols "On"

	filter "configurations:Release"
		defines "DL_RELEASE"
		optimize "On"

	filter "configurations:Production"
		defines "DL_PRODUCTION"
		optimize "On"
---------------------------------------------------------------------------------------------------

project "dlUnitTests"
	location "dlUnitTests"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"

	targetdir("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/source/**.h",
		"%{prj.name}/source/**.cpp"
	}

	includedirs
	{
		"dlEngine/source",
		"ThirdParty/googletest/googletest/include"
	}

	links
	{
		"dlEngine",
		"ThirdParty/lib/googletest/%{cfg.buildcfg}/gtest.lib",
		"ThirdParty/lib/googletest/%{cfg.buildcfg}/gtest_main.lib"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"DL_PLATFORM_WINDOWS",
		}

	filter "configurations:Debug"
		defines "DL_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "DL_RELEASE"
		optimize "On"

	filter "configurations:Production"
		defines "DL_PRODUCTION"
		optimize "On"