#ifdef DL_PLATFORM_LINUX
#include "LinuxResourcesMonitor.h"

using namespace Daedalus;

float LinuxResourcesMonitor::GetAppCPUUsage()
{
	return 0.f;
}

float LinuxResourcesMonitor::GetTotalCPUUsage()
{
	return 0.f;
}

float LinuxResourcesMonitor::GetAppRAMUsage()
{
	return 0.f;
}

float LinuxResourcesMonitor::GetTotalRAMUsage()
{
	return 0.f;
}
#endif