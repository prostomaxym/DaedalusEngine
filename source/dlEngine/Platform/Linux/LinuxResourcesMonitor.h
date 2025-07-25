#pragma once
#ifdef DL_PLATFORM_LINUX
namespace Daedalus
{
	class LinuxResourcesMonitor
	{
	public:
		static float GetAppCPUUsage();
		static float GetTotalCPUUsage();
		static float GetAppRAMUsage();
		static float GetTotalRAMUsage();
	};
}
#endif