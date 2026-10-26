#pragma once

#include <filesystem>

#include "Macros.h"

namespace Daedalus
{
	class DAEDALUS_API GraphicsConfig
	{
	public:
		GraphicsConfig();
		~GraphicsConfig();
		static void Load(const std::filesystem::path& path) { GetInstance().LoadImpl(path); }
		static void Save(const std::filesystem::path& path) { GetInstance().SaveImpl(path); }
		static void LoadDefault() { GetInstance().LoadDefaultImpl(); }
		
		static std::string GetWindowTitle() { return GetInstance().m_window_name; }
		static int GetWindowWidth() { return GetInstance().m_window_width; }
		static int GetWindowHeight() { return GetInstance().m_window_height; }
		static int GetFPSLock() { return GetInstance().m_FPSLock; }
		static bool IsWindowFullscreen() { return GetInstance().m_window_fullscreen; }
		static bool IsVSyncEnabled() { return GetInstance().m_VSync; }
		
		static int GetShadowBufferWidth() { return GetInstance().m_shadow_buffer_width; }
		static int GetShadowBufferHeight() { return GetInstance().m_shadow_buffer_height; }
		static int GetShadowBufferSamples() { return GetInstance().m_shadow_buffer_samples; }
		static int GetShadowPCFMultiplier() { return GetInstance().m_shadow_pcf_multiplier; }
		static float GetShadowCSMExponent() { return GetInstance().m_shadow_csm_exponent; }

		static bool IsSSBOEnabled() { return GetInstance().m_ssao_enabled; }
		static float GetSSAORadius() { return GetInstance().m_ssao_radius; }
		static float GetSSAOBias() { return GetInstance().m_ssao_bias; }
		static int GetSSAOKernelSize() { return GetInstance().m_ssao_kernel_size; }

		static bool IsGammaCorrectionEnabled() { return GetInstance().m_gamma_correction_enabled; }
		static float GetGammaCorrectionValue() { return GetInstance().m_gamma_correction_value; }
		
		static bool IsMultisampleEnabled() { return GetInstance().m_multisample_enabled; }
		static bool IsBlendingEnabled() { return GetInstance().m_blending_enabled; }
		static bool IsLineSmoothEnabled() { return GetInstance().m_line_smooth_enabled; }
		static bool RecompilingShadersEnabled() { return GetInstance().m_recompile_shaders; }
		
	private:
		GraphicsConfig(const GraphicsConfig&) = delete;
		GraphicsConfig& operator=(const GraphicsConfig&) = delete;
		GraphicsConfig(GraphicsConfig&&) = delete;
		GraphicsConfig& operator=(GraphicsConfig&&) = delete;

		static GraphicsConfig& GetInstance();
		void LoadImpl(const std::filesystem::path& path);
		void SaveImpl(const std::filesystem::path& path);
		void LoadDefaultImpl();
		static GraphicsConfig* s_instance;

		std::string m_window_name;
		int m_window_width;
		int m_window_height;
		int m_FPSLock;
		bool m_window_fullscreen;
		bool m_VSync;
		
		int m_shadow_buffer_width;
		int m_shadow_buffer_height;
		int m_shadow_buffer_samples;
		int m_shadow_pcf_multiplier;
		float m_shadow_csm_exponent;

		bool m_ssao_enabled;
		float m_ssao_radius;
		float m_ssao_bias;
		int  m_ssao_kernel_size;

		bool m_gamma_correction_enabled;
		float m_gamma_correction_value;
		
		bool m_multisample_enabled;
		bool m_blending_enabled;
		bool m_line_smooth_enabled;
		bool m_recompile_shaders;
	};
}