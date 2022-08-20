workspace "Daedalus"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Production"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"


project "dlEngine"
	location "dlEngine"
	kind "SharedLib"
	language "C++"

	targetdir("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/source/**.h",
		"%{prj.name}/source/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/source",
		"%{prj.name}/ThirdParty/spdlog/include"
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
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/dlSandbox")
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


project "dlSandbox"
	location "dlSandbox"
	kind "ConsoleApp"
	language "C++"

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
		symbols "On"

	filter "configurations:Release"
		defines "DL_RELEASE"
		optimize "On"

	filter "configurations:Production"
		defines "DL_PRODUCTION"
		optimize "On"