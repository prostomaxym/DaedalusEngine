#pragma once

#include <filesystem>

#include "Daedalus/Core/Core.h"

namespace Daedalus
{
	class DAEDALUS_API GraphicsConfig
	{
	public:
		static void Load(const std::filesystem::path& path);
		static void Save(const std::filesystem::path& path);
		static void LoadDefault();
		
		static std::string GetWindowTitle() { return s_window_name; }
		static int GetWindowWidth() { return s_window_width; }
		static int GetWindowHeight() { return s_window_height; }
		static int GetFPSLock() { return s_FPSLock; }
		static bool IsWindowFullscreen() { return s_window_fullscreen; }
		static bool IsVSyncEnabled() { return s_VSync; }
		
		static int GetShadowBufferWidth() { return s_shadow_buffer_width; }
		static int GetShadowBufferHeight() { return s_shadow_buffer_height; }
		static int GetShadowBufferSamples() { return s_shadow_buffer_samples; }
		
		static bool IsGammaCorrectionEnabled() { return s_gamma_correction_enabled; }
		static double GetGammaCorrectionValue() { return s_gamma_correction_value; }
		
		static bool IsMultisampleEnabled() { return s_multisample_enabled; }
		static bool IsBlendingEnabled() { return s_blending_enabled; }
		static bool IsLineSmoothEnabled() { return s_line_smooth_enabled; }
		
	private:
		static std::string s_window_name;
		static int s_window_width;
		static int s_window_height;
		static int s_FPSLock;
		static bool s_window_fullscreen;
		static bool s_VSync;
		
		static int s_shadow_buffer_width;
		static int s_shadow_buffer_height;
		static int s_shadow_buffer_samples;
		
		static bool s_gamma_correction_enabled;
		static double s_gamma_correction_value;
		
		static bool s_multisample_enabled;
		static bool s_blending_enabled;
		static bool s_line_smooth_enabled;
	};
}