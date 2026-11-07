#ifdef DL_PLATFORM_WINDOWS
#include "Platform/ResourcesMonitor.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <psapi.h>

#include "Platform/Nvidia/NvidiaGPUMonitor.h"

#include <chrono>
#include <cstring>

namespace
{
	class CpuUsageMonitor
	{
	public:

		CpuUsageMonitor() :
			last_load_total(0.0)
			, last_load_app(0.0)
			, last_query_time_app(std::chrono::steady_clock::now())
			, last_query_time_total(std::chrono::steady_clock::now())
		
		{
			InitAppInfo();
			InitTotalInfo();
		}

		double GetAppLoadPercent()
		{
			auto now_time = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - last_query_time_app);

			if (elapsed.count() < window_interval_ms)
				return last_load_app;

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
			last_load_app = percent * 100;
			last_query_time_app = now_time;

			return last_load_app;
		}

		double GetTotalLoadPercent()
		{
			auto now_time = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - last_query_time_total);

			if (elapsed.count() < window_interval_ms)
				return last_load_total;

			PDH_FMT_COUNTERVALUE counterVal;

			PdhCollectQueryData(cpuQuery);
			PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
			last_query_time_total = now_time;
			last_load_total = counterVal.doubleValue;
			return last_load_total;
		}

		const std::string& GetProcessorName() const { return processor_name; }

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

			HKEY hKey;
			const char* subkey = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
			char value[256];
			DWORD size = sizeof(value);

			if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
				processor_name = "Unknown CPU";

			if (RegQueryValueExA(hKey, "ProcessorNameString", nullptr, nullptr, reinterpret_cast<LPBYTE>(value), &size) != ERROR_SUCCESS)
			{
				RegCloseKey(hKey);
				processor_name = "Unknown CPU";
			}

			processor_name = value;

			RegCloseKey(hKey);

			PdhOpenQuery(NULL, NULL, &cpuQuery);
			// You can also use L"\\Processor(*)\\% Processor Time" and get individual CPU values with PdhGetFormattedCounterArray()
			PdhAddEnglishCounter(cpuQuery, "\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
			PdhCollectQueryData(cpuQuery);
		}

		ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
		int numProcessors{ 0 };
		HANDLE self{ nullptr };
		std::string processor_name;

		PDH_HQUERY cpuQuery{ nullptr };
		PDH_HCOUNTER cpuTotal{ nullptr };
		std::chrono::steady_clock::time_point last_query_time_total;
		std::chrono::steady_clock::time_point last_query_time_app;
		double last_load_total;
		double last_load_app;

		const int window_interval_ms{ 1000 };
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

namespace Daedalus
{
	float ResourcesMonitor::GetAppCPUUsage()
	{
		return cpu_monitor.GetAppLoadPercent();
	}

	float ResourcesMonitor::GetTotalCPUUsage()
	{
		return cpu_monitor.GetTotalLoadPercent();
	}

	float ResourcesMonitor::GetAppRAMUsageInMB()
	{
		return ConvertBytesToMB(RAMUsageMonitor::GetAppMemoryUsedInBytes());
	}

	float ResourcesMonitor::GetAppRAMUsageInGB()
	{
		return ConvertBytesToGB(RAMUsageMonitor::GetAppMemoryUsedInBytes());
	}

	float ResourcesMonitor::GetTotalRAMUsageInMB()
	{
		return ConvertBytesToMB(RAMUsageMonitor::GetTotalMemoryUsedInBytes());
	}

	float ResourcesMonitor::GetTotalRAMUsageInGB()
	{
		return ConvertBytesToGB(RAMUsageMonitor::GetTotalMemoryUsedInBytes());
	}

	float ResourcesMonitor::GetSystemRAMInMB()
	{
		return ConvertBytesToMB(RAMUsageMonitor::GetSystemRAMInBytes());
	}

	float ResourcesMonitor::GetSystemRAMInGB()
	{
		return ConvertBytesToGB(RAMUsageMonitor::GetSystemRAMInBytes());
	}

	const std::string& ResourcesMonitor::GetProcessorName()
	{
		return cpu_monitor.GetProcessorName();
	}

	const GpuInfo& ResourcesMonitor::GetGPUInfo()
	{
		return NvidiaGPUMonitor::GetInfo();
	}

	const std::string& ResourcesMonitor::GetDeviceDescription()
	{
		static const std::string description = []
		{
			SYSTEM_INFO info{};
			GetNativeSystemInfo(&info);
			const char* architecture = "Unknown architecture";
			switch (info.wProcessorArchitecture)
			{
			case PROCESSOR_ARCHITECTURE_AMD64: architecture = "x64"; break;
			case PROCESSOR_ARCHITECTURE_INTEL: architecture = "x86"; break;
			case PROCESSOR_ARCHITECTURE_ARM64: architecture = "ARM64"; break;
			case PROCESSOR_ARCHITECTURE_ARM: architecture = "ARM"; break;
			}
			return std::string("Windows / ") + architecture + "\n" + GetProcessorName();
		}();
		return description;
	}
}
#endif