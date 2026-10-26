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

GraphicsConfig* GraphicsConfig::s_instance = nullptr;

GraphicsConfig::GraphicsConfig()
{
	DL_ASSERT(!s_instance, Log::Categories::Application, "Only one graphics configuration may be active");
	if (s_instance) std::abort();
	s_instance = this;

	m_window_name = "Daedalus Engine";

	m_window_width = 1920;
	m_window_height = 1080;
	m_FPSLock = 0;
	m_window_fullscreen = true;
	m_VSync = false;
		
	m_shadow_buffer_width = 2048;
	m_shadow_buffer_height = 2048;
	m_shadow_buffer_samples = 1;
	m_shadow_pcf_multiplier = 5;
	m_shadow_csm_exponent = 1.5f;

	m_ssao_enabled = true;
	m_ssao_radius = 0.5f;
	m_ssao_bias = 0.025f;
	m_ssao_kernel_size = 64;

	m_gamma_correction_enabled = false;
	m_gamma_correction_value = 2.2f;

	m_multisample_enabled = true;
	m_blending_enabled = true;
	m_line_smooth_enabled = true;
	m_recompile_shaders = false;
}

GraphicsConfig::~GraphicsConfig()
{
	s_instance = nullptr;
}

GraphicsConfig& GraphicsConfig::GetInstance()
{
	DL_ASSERT(s_instance, Log::Categories::Application, "GraphicsConfig used outside its lifetime");
	if (!s_instance) std::abort();
	return *s_instance;
}

void GraphicsConfig::LoadImpl(const std::filesystem::path& path)
{
	CSimpleIniA ini;
    ini.SetUnicode();

    SI_Error rc = ini.LoadFile(path.string().c_str());
    if (rc < 0)
	{
		Log::Write(Log::Levels::Warn, Log::Categories::Application, "Failed to load graphic config from {}\nLoading default config", path.string());
        LoadDefault();
		return;
    }
	else
	{
		Log::Write(Log::Levels::Info, Log::Categories::Application, "Loaded graphics config from {}", path.string());
	}

	m_window_name = ini.GetValue(WindowSectionName, WindowName, "Daedalus Engine");
    m_window_width = ini.GetLongValue(WindowSectionName, WindowWidthName, 1920);
    m_window_height = ini.GetLongValue(WindowSectionName, WindowHeightName, 1080);
    m_window_fullscreen = ini.GetBoolValue(WindowSectionName, WindowFullscreenName, true);
    m_FPSLock = ini.GetLongValue(WindowSectionName, FPSLockName, 0);
	m_VSync = ini.GetBoolValue(WindowSectionName, WindowVSyncName, false);

    m_shadow_buffer_width = ini.GetLongValue(ShadowSectionName, ShadowBufferWidthName, 2048);
    m_shadow_buffer_height = ini.GetLongValue(ShadowSectionName, ShadowBufferHeightName, 2048);
    m_shadow_buffer_samples = ini.GetLongValue(ShadowSectionName, ShadowBufferSamplesName, 1);
	m_shadow_pcf_multiplier = ini.GetLongValue(ShadowSectionName, ShadowPCFMultiplierName, 5);
	m_shadow_csm_exponent = static_cast<float>(ini.GetDoubleValue(ShadowSectionName, ShadowCSMCascadeExpName, 1.5));

	m_ssao_enabled = ini.GetBoolValue(SSAOSectionName, SSAOEnabledName, true);
	m_ssao_radius = static_cast<float>(ini.GetDoubleValue(SSAOSectionName, SSAORadiusName, 0.5));
	m_ssao_bias = static_cast<float>(ini.GetDoubleValue(SSAOSectionName, SSAOBiasName, 0.025));
	m_ssao_kernel_size = ini.GetLongValue(SSAOSectionName, SSAOKernelSizeName, 64);

    m_gamma_correction_enabled = ini.GetBoolValue(RenderingSectionName, GammaCorrectionEnabledName, false);
    m_gamma_correction_value = static_cast<float>(ini.GetDoubleValue(RenderingSectionName, GammaCorrectionValueName, 2.2));
	
	m_multisample_enabled = ini.GetBoolValue(RenderingSectionName, MultisampleEnabledName, true);
	m_blending_enabled = ini.GetBoolValue(RenderingSectionName, BlendingEnabledName, true);
	m_line_smooth_enabled = ini.GetBoolValue(RenderingSectionName, LineSmoothEnabledName, true);
	m_recompile_shaders = ini.GetBoolValue(RenderingSectionName, RecompileShadersName, false);
}

void GraphicsConfig::SaveImpl(const std::filesystem::path& path)
{
	CSimpleIniA ini;
    ini.SetUnicode();

	ini.SetValue(WindowSectionName, WindowName, m_window_name.c_str());
    ini.SetLongValue(WindowSectionName, WindowWidthName, m_window_width);
    ini.SetLongValue(WindowSectionName, WindowHeightName, m_window_height);
    ini.SetBoolValue(WindowSectionName, WindowFullscreenName, m_window_fullscreen);
    ini.SetLongValue(WindowSectionName, FPSLockName, m_FPSLock);
	ini.SetBoolValue(WindowSectionName, WindowVSyncName, m_VSync);

    ini.SetLongValue(ShadowSectionName, ShadowBufferWidthName, m_shadow_buffer_width);
    ini.SetLongValue(ShadowSectionName, ShadowBufferHeightName, m_shadow_buffer_height);
    ini.SetLongValue(ShadowSectionName, ShadowBufferSamplesName, m_shadow_buffer_samples);
	ini.SetLongValue(ShadowSectionName, ShadowPCFMultiplierName, m_shadow_pcf_multiplier);
	ini.SetDoubleValue(ShadowSectionName, ShadowCSMCascadeExpName, m_shadow_csm_exponent);

	ini.SetBoolValue(SSAOSectionName, SSAOEnabledName, m_ssao_enabled);
	ini.SetDoubleValue(SSAOSectionName, SSAORadiusName, m_ssao_radius);
	ini.SetDoubleValue(SSAOSectionName, SSAOBiasName, m_ssao_bias);
	ini.SetLongValue(SSAOSectionName, SSAOKernelSizeName, m_ssao_kernel_size);

    ini.SetBoolValue(RenderingSectionName, GammaCorrectionEnabledName, m_gamma_correction_enabled);
    ini.SetDoubleValue(RenderingSectionName, GammaCorrectionValueName, static_cast<double>(m_gamma_correction_value));

	ini.SetBoolValue(RenderingSectionName, MultisampleEnabledName, m_multisample_enabled);
	ini.SetBoolValue(RenderingSectionName, BlendingEnabledName, m_blending_enabled);
	ini.SetBoolValue(RenderingSectionName, LineSmoothEnabledName, m_line_smooth_enabled);
	ini.SetBoolValue(RenderingSectionName, RecompileShadersName, m_recompile_shaders);

	if (!std::filesystem::exists(path.parent_path()))
		std::filesystem::create_directory(path.parent_path());

    SI_Error rc = ini.SaveFile(path.string().c_str());
    if (rc < 0) 
	{
		Log::Write(Log::Levels::Warn, Log::Categories::Application, "Failed to save graphics config to {}\n", path.string());
    }
	else
	{
		Log::Write(Log::Levels::Info, Log::Categories::Application, "Saved graphics config to {}", path.string());
	}
}

void GraphicsConfig::LoadDefaultImpl()
{
	m_window_name = "Daedalus Engine";

	const auto [monitor_width, monitor_height] = Platform::GetMonitorResolution();
	m_window_width = monitor_width;
	m_window_height = monitor_height;
	m_FPSLock = 0;
	m_window_fullscreen = true;
	m_VSync = false;
		
	m_shadow_buffer_width = 2048;
	m_shadow_buffer_height = 2048;
	m_shadow_buffer_samples = 1;
	m_shadow_pcf_multiplier = 5;
	m_shadow_csm_exponent = 1.5f;

	m_ssao_enabled = true;
	m_ssao_radius = 0.5f;
	m_ssao_bias = 0.025f;
	m_ssao_kernel_size = 64;

	m_gamma_correction_enabled = false;
	m_gamma_correction_value = 2.2f;
	
	m_multisample_enabled = true;
	m_blending_enabled = true;
	m_line_smooth_enabled = true;
	m_recompile_shaders = false;
}