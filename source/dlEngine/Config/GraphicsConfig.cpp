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
	constexpr auto GammaCorrectionEnabledName = "GammaCorrectionEnabled";
	constexpr auto GammaCorrectionValueName = "GammaCorrectionValue";
	constexpr auto MultisampleEnabledName = "MultisampleEnabled";
	constexpr auto BlendingEnabledName = "BlendingEnabled";
	constexpr auto LineSmoothEnabledName = "LineSmoothEnabled";
	constexpr auto RecompileShadersName = "RecompileShaders";

	constexpr auto ShadowSectionName = "Shadows";
	constexpr auto ShadowBufferWidthName = "ShadowBufferWidth";
	constexpr auto ShadowBufferHeightName = "ShadowBufferHeight";
	constexpr auto ShadowBufferSamplesName = "ShadowBufferSamples";
	constexpr auto ShadowPCFMultiplierName = "ShadowPCFMultiplier";
	constexpr auto ShadowCSMCascadeExpName = "ShadowCSMExponent";

	constexpr auto SSAOSectionName = "SSAO";
	constexpr auto SSAOEnabledName = "SSAOEnabled";
	constexpr auto SSAORadiusName = "SSAORadius";
	constexpr auto SSAOBiasName = "SSAOBias";
	constexpr auto SSAOKernelSizeName = "SSAOKernelSize";

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
int GraphicsConfig::s_shadow_pcf_multiplier = 5;
float GraphicsConfig::s_shadow_csm_exponent = 1.5f;

bool GraphicsConfig::s_ssao_enabled = true;
float GraphicsConfig::s_ssao_radius = 0.5f;
float GraphicsConfig::s_ssao_bias = 0.025f;
int GraphicsConfig::s_ssao_kernel_size = 64;

bool GraphicsConfig::s_gamma_correction_enabled = false;
float GraphicsConfig::s_gamma_correction_value = 2.2f;
		
bool GraphicsConfig::s_multisample_enabled = true;
bool GraphicsConfig::s_blending_enabled = true;
bool GraphicsConfig::s_line_smooth_enabled = true;
bool GraphicsConfig::s_recompile_shaders = false;

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

    s_shadow_buffer_width = ini.GetLongValue(ShadowSectionName, ShadowBufferWidthName, 2048);
    s_shadow_buffer_height = ini.GetLongValue(ShadowSectionName, ShadowBufferHeightName, 2048);
    s_shadow_buffer_samples = ini.GetLongValue(ShadowSectionName, ShadowBufferSamplesName, 1);
	s_shadow_pcf_multiplier = ini.GetLongValue(ShadowSectionName, ShadowPCFMultiplierName, 5);
	s_shadow_csm_exponent = static_cast<float>(ini.GetDoubleValue(ShadowSectionName, ShadowCSMCascadeExpName, 1.5));

	s_ssao_enabled = ini.GetBoolValue(SSAOSectionName, SSAOEnabledName, true);
	s_ssao_radius = static_cast<float>(ini.GetDoubleValue(SSAOSectionName, SSAORadiusName, 0.5));
	s_ssao_bias = static_cast<float>(ini.GetDoubleValue(SSAOSectionName, SSAOBiasName, 0.025));
	s_ssao_kernel_size = ini.GetLongValue(SSAOSectionName, SSAOKernelSizeName, 64);

    s_gamma_correction_enabled = ini.GetBoolValue(RenderingSectionName, GammaCorrectionEnabledName, false);
    s_gamma_correction_value = static_cast<float>(ini.GetDoubleValue(RenderingSectionName, GammaCorrectionValueName, 2.2));
	
	s_multisample_enabled = ini.GetBoolValue(RenderingSectionName, MultisampleEnabledName, true);
	s_blending_enabled = ini.GetBoolValue(RenderingSectionName, BlendingEnabledName, true);
	s_line_smooth_enabled = ini.GetBoolValue(RenderingSectionName, LineSmoothEnabledName, true);
	s_recompile_shaders = ini.GetBoolValue(RenderingSectionName, RecompileShadersName, false);
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

    ini.SetLongValue(ShadowSectionName, ShadowBufferWidthName, s_shadow_buffer_width);
    ini.SetLongValue(ShadowSectionName, ShadowBufferHeightName, s_shadow_buffer_height);
    ini.SetLongValue(ShadowSectionName, ShadowBufferSamplesName, s_shadow_buffer_samples);
	ini.SetLongValue(ShadowSectionName, ShadowPCFMultiplierName, s_shadow_pcf_multiplier);
	ini.SetDoubleValue(ShadowSectionName, ShadowCSMCascadeExpName, s_shadow_csm_exponent);

	ini.SetBoolValue(SSAOSectionName, SSAOEnabledName, s_ssao_enabled);
	ini.SetDoubleValue(SSAOSectionName, SSAORadiusName, s_ssao_radius);
	ini.SetDoubleValue(SSAOSectionName, SSAOBiasName, s_ssao_bias);
	ini.SetLongValue(SSAOSectionName, SSAOKernelSizeName, s_ssao_kernel_size);

    ini.SetBoolValue(RenderingSectionName, GammaCorrectionEnabledName, s_gamma_correction_enabled);
    ini.SetDoubleValue(RenderingSectionName, GammaCorrectionValueName, static_cast<double>(s_gamma_correction_value));

	ini.SetBoolValue(RenderingSectionName, MultisampleEnabledName, s_multisample_enabled);
	ini.SetBoolValue(RenderingSectionName, BlendingEnabledName, s_blending_enabled);
	ini.SetBoolValue(RenderingSectionName, LineSmoothEnabledName, s_line_smooth_enabled);
	ini.SetBoolValue(RenderingSectionName, RecompileShadersName, s_recompile_shaders);

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
	s_shadow_pcf_multiplier = 5;
	s_shadow_csm_exponent = 1.5f;

	s_ssao_enabled = true;
	s_ssao_radius = 0.5f;
	s_ssao_bias = 0.025f;
	s_ssao_kernel_size = 64;

	s_gamma_correction_enabled = false;
	s_gamma_correction_value = 2.2f;
	
	s_multisample_enabled = true;
	s_blending_enabled = true;
	s_line_smooth_enabled = true;
	s_recompile_shaders = false;
}