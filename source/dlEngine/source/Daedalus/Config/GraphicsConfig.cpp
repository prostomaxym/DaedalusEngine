#include "dlpch.h"

#include "GraphicsConfig.h"
#include <SimpleIni.h>

using namespace Daedalus;

namespace
{
	constexpr auto WindowSectionName = "Window";
	constexpr auto WindowWidthName = "WindowWidth";
	constexpr auto WindowHeightName = "WindowHeight";
	constexpr auto FPSLockName = "FPSLock";
	constexpr auto WindowFullscreenName = "WindowFullscreen";

	constexpr auto RenderingSectionName = "Rendering";
	constexpr auto ShadowBufferWidthName = "ShadowBufferWidth";
	constexpr auto ShadowBufferHeightName = "ShadowBufferHeight";
	constexpr auto ShadowBufferSamplesName = "ShadowBufferSamples";
	constexpr auto GammaCorrectionEnabledName = "GammaCorrectionEnabled";
	constexpr auto GammaCorrectionValueName = "GammaCorrectionValue";
}

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

    m_window_width = ini.GetLongValue(WindowSectionName, WindowWidthName, 1920);
    m_window_height = ini.GetLongValue(WindowSectionName, WindowHeightName, 1080);
    m_window_fullscreen = ini.GetBoolValue(WindowSectionName, WindowFullscreenName, true);
    m_FPSLock = ini.GetLongValue(WindowSectionName, "FPSLock", 0);

    m_shadow_buffer_width = ini.GetLongValue(RenderingSectionName, ShadowBufferWidthName, 2048);
    m_shadow_buffer_height = ini.GetLongValue(RenderingSectionName, ShadowBufferHeightName, 2048);
    m_shadow_buffer_samples = ini.GetLongValue(RenderingSectionName, ShadowBufferSamplesName, 1);
    m_gamma_correction_enabled = ini.GetBoolValue(RenderingSectionName, GammaCorrectionEnabledName, false);
    m_gamma_correction_value = ini.GetDoubleValue(RenderingSectionName, GammaCorrectionValueName, 2.2);
}

void GraphicsConfig::Save(const std::filesystem::path& path)
{
	CSimpleIniA ini;
    ini.SetUnicode();

    ini.SetLongValue(WindowSectionName, WindowWidthName, m_window_width);
    ini.SetLongValue(WindowSectionName, WindowHeightName, m_window_height);
    ini.SetBoolValue(WindowSectionName, WindowFullscreenName, m_window_fullscreen);
    ini.SetLongValue(WindowSectionName, "FPSLock", m_FPSLock);

    ini.SetLongValue(RenderingSectionName, ShadowBufferWidthName, m_shadow_buffer_width);
    ini.SetLongValue(RenderingSectionName, ShadowBufferHeightName, m_shadow_buffer_height);
    ini.SetLongValue(RenderingSectionName, ShadowBufferSamplesName, m_shadow_buffer_samples);
    ini.SetBoolValue(RenderingSectionName, GammaCorrectionEnabledName, m_gamma_correction_enabled);
    ini.SetDoubleValue(RenderingSectionName, GammaCorrectionValueName, m_gamma_correction_value);

    SI_Error rc = ini.SaveFile(path.string().c_str());
    if (rc < 0) 
	{
        Log::Write(Log::Levels::Warn, Log::Categories::Renderer, "Failed to save graphic config to " + path.string() + "\n");
    }
	else
	{
		Log::Write(Log::Levels::Info, Log::Categories::Renderer, "Saved graphics config to " + path.string());
	}
}

void GraphicsConfig::LoadDefault()
{
	m_window_width = 1920;
	m_window_height = 1080;
	m_FPSLock = 0;
	m_window_fullscreen = true;
		
	m_shadow_buffer_width = 2048;
	m_shadow_buffer_height = 2048;
	m_shadow_buffer_samples = 1;
		
	m_gamma_correction_enabled = false;
	m_gamma_correction_value = 2.2;
}