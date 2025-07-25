#ifdef DL_PLATFORM_LINUX
#include "LinuxResourcesMonitor.h"

#include "sys/types.h"
#include "sys/sysinfo.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

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
			long long totalVirtualMem = memInfo.totalram;
			//Add other values in next statement to avoid int overflow on right hand side...
			totalVirtualMem += memInfo.totalswap;
			totalVirtualMem *= memInfo.mem_unit;
			long long totalPhysMem = memInfo.totalram;
			//Multiply in next statement to avoid int overflow on right hand side...
			totalPhysMem *= memInfo.mem_unit

			return totalPhysMem;
		}

		static long long GetAppMemoryUsedInBytes()
		{
			FILE* file = fopen("/proc/self/status", "r");
			int result = -1;
			char line[128];

			while (fgets(line, 128, file) != NULL) {
				if (strncmp(line, "VmRSS:", 6) == 0) {
					result = parseLine(line);
					break;
				}
			}
			fclose(file);
			return result * 1024;
		}

		static long long GetSystemRAMInBytes()
		{
			MEMORYSTATUSEX memInfo = GetMemoryStatus();
			return memInfo.ullTotalPhys;
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
#endif