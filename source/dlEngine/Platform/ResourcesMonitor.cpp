#include "ResourcesMonitor.h"

#include "Linux/LinuxResourcesMonitor.h"
#include "Windows/WindowsResourcesMonitor.h"

using namespace Daedalus;

namespace
{
	float ConvertBytesToMB(float bytes)
	{
		return bytes / (1024.f * 1024.f);
	}
}

float ResourcesMonitor::GetAppCPUUsage()
{
#ifdef DL_PLATFORM_WINDOWS
	return WindowsResourcesMonitor::GetAppCPUUsage();
#elif defined DL_PLATFORM_LINUX //TODO: not implemented
	return LinuxResourcesMonitor::GetAppCPUUsage();
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}

float ResourcesMonitor::GetTotalCPUUsage()
{
#ifdef DL_PLATFORM_WINDOWS
	return WindowsResourcesMonitor::GetTotalCPUUsage();
#elif defined DL_PLATFORM_LINUX //TODO: not implemented
	return LinuxResourcesMonitor::GetTotalCPUUsage();
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}

float ResourcesMonitor::GetAppRAMUsageInMB()
{
#ifdef DL_PLATFORM_WINDOWS
	return ConvertBytesToMB(WindowsResourcesMonitor::GetAppRAMUsage());
#elif defined DL_PLATFORM_LINUX //TODO: not implemented
	return ConvertBytesToMB(LinuxResourcesMonitor::GetAppRAMUsage());
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}

float ResourcesMonitor::GetTotalRAMUsageInMB()
{
#ifdef DL_PLATFORM_WINDOWS
	return ConvertBytesToMB(WindowsResourcesMonitor::GetTotalRAMUsage());
#elif defined DL_PLATFORM_LINUX //TODO: not implemented
	return ConvertBytesToMB(LinuxResourcesMonitor::GetTotalRAMUsage());
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}