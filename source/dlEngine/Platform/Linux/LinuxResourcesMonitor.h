#pragma once
#ifdef DL_PLATFORM_LINUX
namespace Daedalus
{
	class LinuxResourcesMonitor
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