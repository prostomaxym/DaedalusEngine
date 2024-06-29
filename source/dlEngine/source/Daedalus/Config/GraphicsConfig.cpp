#include "dlpch.h"

#include "GraphicsConfig.h"
#include "Platform/Platform.h"

#include <SimpleIni.h>

using namespace Daedalus;

namespace
{
	constexpr auto WindowSectionName = "Window";
	constexpr auto WindowName = "WindowName";
	constexpr auto WindowWidthName = "WindowWidth";
	constexpr auto WindowHeightName = "WindowHeight";
	constexpr auto FPSLockName = "FPSLock";
	constexpr auto WindowFullscreenName = "WindowFullscreen";
	constexpr auto WindowVSyncName = "VSync";

	constexpr auto RenderingSectionName = "Rendering";
	constexpr auto ShadowBufferWidthName = "ShadowBufferWidth";
	constexpr auto ShadowBufferHeightName = "ShadowBufferHeight";
	constexpr auto ShadowBufferSamplesName = "ShadowBufferSamples";
	constexpr auto GammaCorrectionEnabledName = "GammaCorrectionEnabled";
	constexpr auto GammaCorrectionValueName = "GammaCorrectionValue";
	constexpr auto MultisampleEnabledName = "MultisampleEnabled";
	constexpr auto BlendingEnabledName = "BlendingEnabled";
	constexpr auto LineSmoothEnabledName = "LineSmoothEnabled";
}

std::string GraphicsConfig::s_window_name = "Daedalus Engine";
int GraphicsConfig::s_window_width = 1920;
int GraphicsConfig::s_window_height = 1080;
int GraphicsConfig::s_FPSLock = 0;
bool GraphicsConfig::s_window_fullscreen = true;
bool GraphicsConfig::s_VSync = false;
		
int GraphicsConfig::s_shadow_buffer_width = 2048;
int GraphicsConfig::s_shadow_buffer_height = 2048;
int GraphicsConfig::s_shadow_buffer_samples = 1;
		
bool GraphicsConfig::s_gamma_correction_enabled = false;
float GraphicsConfig::s_gamma_correction_value = 2.2f;
		
bool GraphicsConfig::s_multisample_enabled = true;
bool GraphicsConfig::s_blending_enabled = true;
bool GraphicsConfig::s_line_smooth_enabled = true;
		
void GraphicsConfig::Load(const std::filesystem::path& path)
{
	CSimpleIniA ini;
    ini.SetUnicode();

    SI_Error rc = ini.LoadFile(path.string().c_str());
    if (rc < 0)
	{
		Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "Failed to load graphic config from " + path.string() + "\nLoading default config");
        LoadDefault();
		return;
    }
	else
	{
		Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Loaded graphics config from " + path.string());
	}

	s_window_name = ini.GetValue(WindowSectionName, WindowName, "Daedalus Engine");
    s_window_width = ini.GetLongValue(WindowSectionName, WindowWidthName, 1920);
    s_window_height = ini.GetLongValue(WindowSectionName, WindowHeightName, 1080);
    s_window_fullscreen = ini.GetBoolValue(WindowSectionName, WindowFullscreenName, true);
    s_FPSLock = ini.GetLongValue(WindowSectionName, FPSLockName, 0);
	s_VSync = ini.GetBoolValue(WindowSectionName, WindowVSyncName, false);

    s_shadow_buffer_width = ini.GetLongValue(RenderingSectionName, ShadowBufferWidthName, 2048);
    s_shadow_buffer_height = ini.GetLongValue(RenderingSectionName, ShadowBufferHeightName, 2048);
    s_shadow_buffer_samples = ini.GetLongValue(RenderingSectionName, ShadowBufferSamplesName, 1);
    s_gamma_correction_enabled = ini.GetBoolValue(RenderingSectionName, GammaCorrectionEnabledName, false);
    s_gamma_correction_value = static_cast<float>(ini.GetDoubleValue(RenderingSectionName, GammaCorrectionValueName, 2.2));
	
	s_multisample_enabled = ini.GetBoolValue(RenderingSectionName, MultisampleEnabledName, true);
	s_blending_enabled = ini.GetBoolValue(RenderingSectionName, BlendingEnabledName, true);
	s_line_smooth_enabled = ini.GetBoolValue(RenderingSectionName, LineSmoothEnabledName, true);
}

void GraphicsConfig::Save(const std::filesystem::path& path)
{
	CSimpleIniA ini;
    ini.SetUnicode();

	ini.SetValue(WindowSectionName, WindowName, s_window_name.c_str());
    ini.SetLongValue(WindowSectionName, WindowWidthName, s_window_width);
    ini.SetLongValue(WindowSectionName, WindowHeightName, s_window_height);
    ini.SetBoolValue(WindowSectionName, WindowFullscreenName, s_window_fullscreen);
    ini.SetLongValue(WindowSectionName, FPSLockName, s_FPSLock);
	ini.SetBoolValue(WindowSectionName, WindowVSyncName, s_VSync);

    ini.SetLongValue(RenderingSectionName, ShadowBufferWidthName, s_shadow_buffer_width);
    ini.SetLongValue(RenderingSectionName, ShadowBufferHeightName, s_shadow_buffer_height);
    ini.SetLongValue(RenderingSectionName, ShadowBufferSamplesName, s_shadow_buffer_samples);
    ini.SetBoolValue(RenderingSectionName, GammaCorrectionEnabledName, s_gamma_correction_enabled);
    ini.SetDoubleValue(RenderingSectionName, GammaCorrectionValueName, static_cast<double>(s_gamma_correction_value));

	ini.SetBoolValue(RenderingSectionName, MultisampleEnabledName, s_multisample_enabled);
	ini.SetBoolValue(RenderingSectionName, BlendingEnabledName, s_blending_enabled);
	ini.SetBoolValue(RenderingSectionName, LineSmoothEnabledName, s_line_smooth_enabled);
	
	if (!std::filesystem::exists(path.parent_path()))
		std::filesystem::create_directory(path.parent_path());

    SI_Error rc = ini.SaveFile(path.string().c_str());
    if (rc < 0) 
	{
        Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "Failed to save graphics config to " + path.string() + "\n");
    }
	else
	{
		Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Saved graphics config to " + path.string());
	}
}

void GraphicsConfig::LoadDefault()
{
	s_window_name = "Daedalus Engine";

	const auto [monitor_width, monitor_height] = Platform::GetMonitorResolution();
	s_window_width = monitor_width;
	s_window_height = monitor_height;
	s_FPSLock = 0;
	s_window_fullscreen = true;
	s_VSync = false;
		
	s_shadow_buffer_width = 2048;
	s_shadow_buffer_height = 2048;
	s_shadow_buffer_samples = 1;
		
	s_gamma_correction_enabled = false;
	s_gamma_correction_value = 2.2f;
	
	s_multisample_enabled = true;
	s_blending_enabled = true;
	s_line_smooth_enabled = true;
}