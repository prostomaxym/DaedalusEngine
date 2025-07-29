#include "ResourcesMonitor.h"

#include "Linux/LinuxResourcesMonitor.h"
#include "Windows/WindowsResourcesMonitor.h"
#include "Nvidia/NvidiaGPUMonitor.h"

using namespace Daedalus;

float Daedalus::ConvertBytesToMB(unsigned long long bytes)
{
	return static_cast<float>(bytes / (1024ull * 1024ull));
}

float Daedalus::ConvertBytesToGB(unsigned long long bytes)
{
	return static_cast<float>(bytes / (1024ull * 1024ull)) / 1024.f;
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
	return ConvertBytesToMB(WindowsResourcesMonitor::GetAppRAMUsageInBytes());
#elif defined DL_PLATFORM_LINUX //TODO: not implemented
	return ConvertBytesToMB(LinuxResourcesMonitor::GetAppRAMUsage());
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}

float ResourcesMonitor::GetTotalRAMUsageInMB()
{
#ifdef DL_PLATFORM_WINDOWS
	return ConvertBytesToMB(WindowsResourcesMonitor::GetTotalRAMUsageInBytes());
#elif defined DL_PLATFORM_LINUX //TODO: not implemented
	return ConvertBytesToMB(LinuxResourcesMonitor::GetTotalRAMUsage());
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}

float ResourcesMonitor::GetSystemRAMInMB()
{
#ifdef DL_PLATFORM_WINDOWS
	return ConvertBytesToMB(WindowsResourcesMonitor::GetSystemRAMInBytes());
#elif defined DL_PLATFORM_LINUX //TODO: not implemented
	return ConvertBytesToMB(LinuxResourcesMonitor::GetTotalRAMUsage());
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}

float ResourcesMonitor::GetAppRAMUsageInGB()
{
#ifdef DL_PLATFORM_WINDOWS
	return ConvertBytesToGB(WindowsResourcesMonitor::GetAppRAMUsageInBytes());
#elif defined DL_PLATFORM_LINUX //TODO: not implemented
	return ConvertBytesToMB(LinuxResourcesMonitor::GetTotalRAMUsage());
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}

float ResourcesMonitor::GetTotalRAMUsageInGB()
{
#ifdef DL_PLATFORM_WINDOWS
	return ConvertBytesToGB(WindowsResourcesMonitor::GetTotalRAMUsageInBytes());
#elif defined DL_PLATFORM_LINUX //TODO: not implemented
	return ConvertBytesToGB(LinuxResourcesMonitor::GetTotalRAMUsage());
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}


float ResourcesMonitor::GetSystemRAMInGB()
{
#ifdef DL_PLATFORM_WINDOWS
	return ConvertBytesToGB(WindowsResourcesMonitor::GetSystemRAMInBytes());
#elif defined DL_PLATFORM_LINUX //TODO: not implemented
	return ConvertBytesToGB(LinuxResourcesMonitor::GetSystemRAMInBytes());
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}

const std::string& Daedalus::ResourcesMonitor::GetProcessorName()
{
#ifdef DL_PLATFORM_WINDOWS
	return WindowsResourcesMonitor::GetProcessorName();
#elif defined DL_PLATFORM_LINUX //TODO: not implemented
	return ConvertBytesToGB(LinuxResourcesMonitor::GetProcessorName();
#else
	static_assert(false, "Unsupported Platfrom")
#endif
}

const GpuInfo& ResourcesMonitor::GetGPUInfo()
{
	// As I have only Nvidia GPUs I do not bother to implement for Intel and AMD, sorry..
	return NvidiaGPUMonitor::GetInfo();
}
