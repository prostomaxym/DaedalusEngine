#ifdef DL_PLATFORM_WINDOWS
#include "WindowsResourcesMonitor.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <psapi.h>
#include <thread>

namespace
{
	class CpuUsageMonitor
	{
	public:

		CpuUsageMonitor()
		{
			InitAppInfo();
			InitTotalInfo();
		}

		double GetAppLoadPercent()
		{
			FILETIME ftime, fsys, fuser;
			ULARGE_INTEGER now, sys, user;
			double percent;

			GetSystemTimeAsFileTime(&ftime);
			memcpy(&now, &ftime, sizeof(FILETIME));

			GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
			memcpy(&sys, &fsys, sizeof(FILETIME));
			memcpy(&user, &fuser, sizeof(FILETIME));
			percent = (sys.QuadPart - lastSysCPU.QuadPart) +
				(user.QuadPart - lastUserCPU.QuadPart);
			percent /= (now.QuadPart - lastCPU.QuadPart);
			percent /= numProcessors;
			lastCPU = now;
			lastUserCPU = user;
			lastSysCPU = sys;

			return percent * 100;
		}

		double GetTotalLoadPercent()
		{
			PDH_FMT_COUNTERVALUE counterVal;

			PdhCollectQueryData(cpuQuery);
			PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
			return counterVal.doubleValue;
		}

	private:
		void InitAppInfo()
		{
			SYSTEM_INFO sysInfo;
			FILETIME ftime, fsys, fuser;

			GetSystemInfo(&sysInfo);
			numProcessors = sysInfo.dwNumberOfProcessors;

			GetSystemTimeAsFileTime(&ftime);
			memcpy(&lastCPU, &ftime, sizeof(FILETIME));

			self = GetCurrentProcess();
			GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
			memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
			memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
		}

		void InitTotalInfo()
		{
			PdhOpenQuery(NULL, NULL, &cpuQuery);
			// You can also use L"\\Processor(*)\\% Processor Time" and get individual CPU values with PdhGetFormattedCounterArray()
			PdhAddEnglishCounter(cpuQuery, "\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
			PdhCollectQueryData(cpuQuery);
		}

		ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
		int numProcessors{ 0 };
		HANDLE self{ nullptr };

		PDH_HQUERY cpuQuery{ nullptr };
		PDH_HCOUNTER cpuTotal{ nullptr };
	};

	class RAMUsageMonitor
	{
	public:
		static DWORDLONG GetTotalMemoryUsedInBytes()
		{
			MEMORYSTATUSEX memInfo = GetMemoryStatus();
			DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
			return physMemUsed;
		}

		static DWORDLONG GetAppMemoryUsedInBytes()
		{
			PROCESS_MEMORY_COUNTERS_EX pmc;
			GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
			SIZE_T virtualMemUsedByMe = pmc.PrivateUsage;
			SIZE_T physMemUsedByMe = pmc.WorkingSetSize;
			return physMemUsedByMe;
		}

		static DWORDLONG GetSystemRAMInBytes()
		{
			MEMORYSTATUSEX memInfo = GetMemoryStatus();
			return memInfo.ullTotalPhys;
		}

	private:
		static MEMORYSTATUSEX GetMemoryStatus()
		{
			MEMORYSTATUSEX memInfo;
			memInfo.dwLength = sizeof(MEMORYSTATUSEX);
			GlobalMemoryStatusEx(&memInfo);
			return memInfo;
		}
	};

    CpuUsageMonitor cpu_monitor;
}

using namespace Daedalus;

float WindowsResourcesMonitor::GetAppCPUUsage()
{
	return cpu_monitor.GetAppLoadPercent();
}

float WindowsResourcesMonitor::GetTotalCPUUsage()
{
	return cpu_monitor.GetTotalLoadPercent();
}

unsigned long long WindowsResourcesMonitor::GetAppRAMUsageInBytes()
{
	return RAMUsageMonitor::GetAppMemoryUsedInBytes();
}

unsigned long long WindowsResourcesMonitor::GetTotalRAMUsageInBytes()
{
	return RAMUsageMonitor::GetTotalMemoryUsedInBytes();
}

unsigned long long WindowsResourcesMonitor::GetSystemRAMInBytes()
{
	return RAMUsageMonitor::GetSystemRAMInBytes();
}
#endif