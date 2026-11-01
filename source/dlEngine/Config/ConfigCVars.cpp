#include "dlpch.h"
#include "CVarRegistry.h"
#include "GraphicsConfig.h"
#include "KeybindConfig.h"
#include "PathConfig.h"
#include "Utils/KeyCodes.h"

namespace Daedalus
{
	void RegisterConfigCVars(CVarRegistry& registry)
	{
		const auto bind = [&registry](const char* name, const char* description, CVarRegistry::Getter getter, CVarRegistry::Setter setter = {})
		{
			if (!registry.Register(name, description, std::move(getter), std::move(setter)))
				Log::Write(Log::Levels::Warn, Log::Categories::Application, "Cannot register Config CVar: {}", name);
		};
		bind("graphics.fps_lock", "Frame limit; 0 disables limiting", GraphicsConfig::GetFPSLock,
			[](const CVarRegistry::Value& value) { return GraphicsConfig::SetFPSLock(std::get<int>(value)); });
		bind("graphics.window_title", "Window title", GraphicsConfig::GetWindowTitle);
		bind("graphics.window_width", "Initial window width", GraphicsConfig::GetWindowWidth);
		bind("graphics.window_height", "Initial window height", GraphicsConfig::GetWindowHeight);
		bind("graphics.fullscreen", "Initial screen mode", GraphicsConfig::IsWindowFullscreen);
		bind("graphics.vsync", "Initial swap interval", GraphicsConfig::IsVSyncEnabled);
		bind("graphics.shadow_width", "Shadow buffer width", GraphicsConfig::GetShadowBufferWidth);
		bind("graphics.shadow_height", "Shadow buffer height", GraphicsConfig::GetShadowBufferHeight);
		bind("graphics.shadow_samples", "Shadow buffer samples", GraphicsConfig::GetShadowBufferSamples);
		bind("graphics.shadow_pcf", "Shadow PCF multiplier", GraphicsConfig::GetShadowPCFMultiplier);
		bind("graphics.shadow_csm_exponent", "Shadow cascade exponent", GraphicsConfig::GetShadowCSMExponent);
		bind("graphics.ssao", "SSAO enabled", GraphicsConfig::IsSSBOEnabled);
		bind("graphics.ssao_radius", "SSAO radius", GraphicsConfig::GetSSAORadius);
		bind("graphics.ssao_bias", "SSAO bias", GraphicsConfig::GetSSAOBias);
		bind("graphics.ssao_kernel_size", "SSAO kernel size", GraphicsConfig::GetSSAOKernelSize);
		bind("graphics.gamma", "Gamma correction enabled", GraphicsConfig::IsGammaCorrectionEnabled);
		bind("graphics.gamma_value", "Gamma correction value", GraphicsConfig::GetGammaCorrectionValue);
		bind("graphics.multisample", "Multisampling enabled", GraphicsConfig::IsMultisampleEnabled);
		bind("graphics.blending", "Configured blending flag", GraphicsConfig::IsBlendingEnabled);
		bind("graphics.line_smooth", "Configured line smoothing flag", GraphicsConfig::IsLineSmoothEnabled);
		bind("graphics.recompile_shaders", "Startup shader recompilation", GraphicsConfig::RecompilingShadersEnabled);
		bind("paths.assets", "Assets mount", [] { return Utf8ToString(PathConfig::GetAssetsPath().u8string()); });
		bind("paths.shaders", "Shaders mount", [] { return Utf8ToString(PathConfig::GetShadersPath().u8string()); });
		bind("paths.config", "Config mount", [] { return Utf8ToString(PathConfig::GetConfigPath().u8string()); });
		bind("paths.logs", "Logs mount", [] { return Utf8ToString(PathConfig::GetLogsPath().u8string()); });
		bind("paths.scene", "Configured scene asset", PathConfig::GetSceneAssetPath);
		for (const auto& [name, code] : KeybindConfig::GetKeyboardBinds())
		{
			const auto cvar_name = "keybind." + name;
			bind(cvar_name.c_str(), "GLFW keyboard key code", [name] { return KeybindConfig::GetKeyboardBind(name); },
				[name](const CVarRegistry::Value& value)
				{
					const int key = std::get<int>(value);
					const bool valid = key == DL_KEY_SPACE || key == DL_KEY_APOSTROPHE || (key >= DL_KEY_COMMA && key <= DL_KEY_9) ||
						key == DL_KEY_SEMICOLON || key == DL_KEY_EQUAL || (key >= DL_KEY_A && key <= DL_KEY_RIGHT_BRACKET) ||
						key == DL_KEY_GRAVE_ACCENT || key == DL_KEY_WORLD_1 || key == DL_KEY_WORLD_2 ||
						(key >= DL_KEY_ESCAPE && key <= DL_KEY_END) || (key >= DL_KEY_CAPS_LOCK && key <= DL_KEY_PAUSE) ||
						(key >= DL_KEY_F1 && key <= DL_KEY_F25) || (key >= DL_KEY_KP_0 && key <= DL_KEY_KP_EQUAL) ||
						(key >= DL_KEY_LEFT_SHIFT && key <= DL_KEY_MENU);
					if (!valid) return false;
					KeybindConfig::SetKeyboardBind(name, key);
					return true;
				});
		}
	}
}
