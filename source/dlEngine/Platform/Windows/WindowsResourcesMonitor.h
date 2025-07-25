#pragma once
#ifdef DL_PLATFORM_WINDOWS
namespace Daedalus
{
	class WindowsResourcesMonitor
	{
	public:
		static float GetAppCPUUsage();
		static float GetTotalCPUUsage();
		static float GetAppRAMUsage();
		static float GetTotalRAMUsage();
	};
}
#endif