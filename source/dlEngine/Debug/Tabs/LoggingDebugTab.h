#pragma once

#include "Debug/Log.h"

#include <array>

namespace Daedalus
{
	class LoggingDebugTab
	{
	public:
		void Render();

	private:
		void RenderLogsPage();

		class LogFilterPanel
		{
		public:
			LogFilterPanel();
			void Draw();
			bool IsLevelEnabled(Log::Levels level) const;
			bool IsCategoryEnabled(Log::Categories category_bit) const;

		private:
			std::array<bool, 5> level_flags_;
			void DrawCategoryCheckbox(Log::Categories cat);
			void DrawLevelCheckbox(Log::Levels level);
		};

		LogFilterPanel m_log_selection_panel;
	};
}
