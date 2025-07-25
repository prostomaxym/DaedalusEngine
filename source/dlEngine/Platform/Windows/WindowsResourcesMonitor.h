#pragma once
#ifdef DL_PLATFORM_WINDOWS
namespace Daedalus
{
	class WindowsResourcesMonitor
	{
	public:
		static float GetAppCPUUsage();
		static float GetTotalCPUUsage();

		static unsigned long long GetAppRAMUsageInBytes();
		static unsigned long long GetTotalRAMUsageInBytes();
		static unsigned long long GetSystemRAMInBytes();
	};
}
#endif