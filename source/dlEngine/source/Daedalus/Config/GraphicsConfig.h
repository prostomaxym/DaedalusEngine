#pragma once

#include "Config.h"

namespace Daedalus
{
	class GraphicsConfig final : public Config
	{
	public:
		void Load(const std::filesystem::path& path) override;
		void Save(const std::filesystem::path& path) override;
		void LoadDefault() override;
		
	private:
		int m_window_width{1920};
		int m_window_height{1080};
		int m_FPSLock{0};
		bool m_window_fullscreen{true};
		
		int m_shadow_buffer_width{2048};
		int m_shadow_buffer_height{2048};
		int m_shadow_buffer_samples{1};
		
		bool m_gamma_correction_enabled{false};
		double m_gamma_correction_value{2.2};
	};
}