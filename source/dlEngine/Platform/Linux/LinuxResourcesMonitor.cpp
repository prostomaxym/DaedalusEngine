#ifdef DL_PLATFORM_LINUX
#include "LinuxResourcesMonitor.h"

#include "sys/types.h"
#include "sys/times.h"
#include "sys/sysinfo.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include <string>

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
			struct tms timeSample;
			clock_t now;
			double percent;

			now = times(&timeSample);
			if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
				timeSample.tms_utime < lastUserCPU)
			{
				//Overflow detection. Just skip this value.
				percent = -1.0;
			}
			else
			{
				percent = (timeSample.tms_stime - lastSysCPU) +
					(timeSample.tms_utime - lastUserCPU);
				percent /= (now - lastCPU);
				percent /= numProcessors;
				percent *= 100;
			}
			lastCPU = now;
			lastSysCPU = timeSample.tms_stime;
			lastUserCPU = timeSample.tms_utime;

			return percent;
		}

		double GetTotalLoadPercent()
		{
			double percent;
			FILE* file;
			unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;

			file = fopen("/proc/stat", "r");
			fscanf(file, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow,
				&totalSys, &totalIdle);
			fclose(file);

			if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow ||
				totalSys < lastTotalSys || totalIdle < lastTotalIdle)
			{
				//Overflow detection. Just skip this value.
				percent = -1.0;
			}
			else
			{
				total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
					(totalSys - lastTotalSys);
				percent = total;
				total += (totalIdle - lastTotalIdle);
				percent /= total;
				percent *= 100;
			}

			lastTotalUser = totalUser;
			lastTotalUserLow = totalUserLow;
			lastTotalSys = totalSys;
			lastTotalIdle = totalIdle;

			return percent;
		}

	private:
		void InitAppInfo()
		{
			FILE* file;
			struct tms timeSample;
			char line[128];

			lastCPU = times(&timeSample);
			lastSysCPU = timeSample.tms_stime;
			lastUserCPU = timeSample.tms_utime;

			file = fopen("/proc/cpuinfo", "r");
			numProcessors = 0;
			while (fgets(line, 128, file) != NULL)
			{
				if (strncmp(line, "processor", 9) == 0) numProcessors++;
			}
			fclose(file);
		}

		void InitTotalInfo()
		{
			FILE* file = fopen("/proc/stat", "r");
			fscanf(file, "cpu %llu %llu %llu %llu", &lastTotalUser, &lastTotalUserLow,
				&lastTotalSys, &lastTotalIdle);
			fclose(file);
		}

		clock_t lastCPU, lastSysCPU, lastUserCPU;
		int numProcessors;
		unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;
	};

	class RAMUsageMonitor
	{
	public:
		static long long GetTotalMemoryUsedInBytes()
		{
			struct sysinfo memInfo;

			sysinfo(&memInfo);
			long long used_phys_mem = memInfo.totalram - memInfo.freeram;
			used_phys_mem *= memInfo.mem_unit;

			return used_phys_mem;
		}

		static long long GetAppMemoryUsedInBytes()
		{
			FILE* file = fopen("/proc/self/status", "r");
			int result = -1;
			char line[128];

			while (fgets(line, 128, file) != NULL) {
				if (strncmp(line, "VmRSS:", 6) == 0) {
					// Format: "VmRSS:   1234 kB"
					result = atoi(line + 6);
					break;
				}
			}
			fclose(file);
			return static_cast<long long>(result) * 1024;
		}

		static long long GetSystemRAMInBytes()
		{
			struct sysinfo memInfo;
			sysinfo(&memInfo);
			return static_cast<long long>(memInfo.totalram) * memInfo.mem_unit;
		}

		static const std::string& GetProcessorName()
		{
			static const std::string name = []()
			{
				std::string result = "Unknown";
				FILE* file = fopen("/proc/cpuinfo", "r");
				if (!file)
				{
					return result;
				}

				char line[256];
				while (fgets(line, sizeof(line), file) != NULL)
				{
					if (strncmp(line, "model name", 10) == 0)
					{
						const char* colon = strchr(line, ':');
						if (colon)
						{
							result = colon + 1;
							const auto start = result.find_first_not_of(" \t");
							const auto end = result.find_last_not_of(" \t\r\n");
							result = start == std::string::npos ? "Unknown" : result.substr(start, end - start + 1);
						}
						break;
					}
				}
				fclose(file);
				return result;
			}();
			return name;
		}

	};

	CpuUsageMonitor cpu_monitor;
}

using namespace Daedalus;

float LinuxResourcesMonitor::GetAppCPUUsage()
{
	return cpu_monitor.GetAppLoadPercent();
}

float LinuxResourcesMonitor::GetTotalCPUUsage()
{
	return cpu_monitor.GetTotalLoadPercent();
}

unsigned long long LinuxResourcesMonitor::GetAppRAMUsageInBytes()
{
	return RAMUsageMonitor::GetAppMemoryUsedInBytes();
}

unsigned long long LinuxResourcesMonitor::GetTotalRAMUsageInBytes()
{
	return RAMUsageMonitor::GetTotalMemoryUsedInBytes();
}

unsigned long long LinuxResourcesMonitor::GetSystemRAMInBytes()
{
	return RAMUsageMonitor::GetSystemRAMInBytes();
}

const std::string& LinuxResourcesMonitor::GetProcessorName()
{
	return RAMUsageMonitor::GetProcessorName();
}
#endif