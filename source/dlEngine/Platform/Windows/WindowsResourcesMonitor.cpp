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
			PdhOpenQuery(nullptr, 0, &query_);
			PdhAddCounter(query_, "\\Processor(_Total)\\% Processor Time", 0, &counter_);
			PdhCollectQueryData(query_);
		}

		~CpuUsageMonitor()
		{
			PdhCloseQuery(query_);
		}

		double Get()
		{
			PDH_FMT_COUNTERVALUE val;
			PdhCollectQueryData(query_);
			PdhGetFormattedCounterValue(counter_, PDH_FMT_DOUBLE, nullptr, &val);
			return val.doubleValue;
		}

	private:
		PDH_HQUERY query_;
		PDH_HCOUNTER counter_;
	};

	class AppCpuUsageMonitor
	{
	public:
		AppCpuUsageMonitor()
		{
			char processName[MAX_PATH] = { 0 };
			GetModuleFileName(nullptr, processName, MAX_PATH);

			std::string name = processName;
			name = name.substr(name.find_last_of("\\") + 1);
			if (EndsWith(name, ".exe"))
				name = name.substr(0, name.size() - 4);

			std::string counterPath = "\\Process(" + name + ")\\% Processor Time";

			PdhOpenQuery(nullptr, 0, &query_);
			PdhAddCounter(query_, counterPath.c_str(), 0, &counter_);
			PdhCollectQueryData(query_);
		}

		~AppCpuUsageMonitor()
		{
			PdhCloseQuery(query_);
		}

		double Get()
		{
			PDH_FMT_COUNTERVALUE val;
			PdhCollectQueryData(query_);
			PdhGetFormattedCounterValue(counter_, PDH_FMT_DOUBLE, nullptr, &val);
			return val.doubleValue / static_cast<double>(std::thread::hardware_concurrency());
		}

	private:
		bool EndsWith(const std::string& str, const std::string& suffix)
		{
			return str.size() >= suffix.size() &&
				str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
		}
		PDH_HQUERY query_;
		PDH_HCOUNTER counter_;
	};

	CpuUsageMonitor cpu_total;
	AppCpuUsageMonitor cpu_app;

	SIZE_T GetUsedRAM()
	{
		PROCESS_MEMORY_COUNTERS memInfo;
		GetProcessMemoryInfo(GetCurrentProcess(), &memInfo, sizeof(memInfo));
		return memInfo.WorkingSetSize;
	}
}

using namespace Daedalus;

float WindowsResourcesMonitor::GetAppCPUUsage()
{
	return cpu_app.Get();
}

float WindowsResourcesMonitor::GetTotalCPUUsage()
{
	return cpu_total.Get();
}

float WindowsResourcesMonitor::GetAppRAMUsage()
{
	return GetUsedRAM();
}

float WindowsResourcesMonitor::GetTotalRAMUsage()
{
	return GetUsedRAM();
}
#endif