#pragma once
#include "Macros.h"

namespace Daedalus
{
	struct GpuInfo
	{
		std::string name;
		std::string driver_version;
		unsigned int temperature_celsius{ 0u };
		unsigned int gpu_utilization_percent{ 0u };
		unsigned int memory_utilization_percent{ 0u };
		unsigned int power_usage_watt{ 0u };
		unsigned int graphics_clock_mhz{ 0u };
		unsigned int memory_clock_mhz{ 0u };
		float memory_total_GB{ 0.f };
		float memory_used_GB{ 0.f };
	};

	float ConvertBytesToMB(unsigned long long bytes);
	float ConvertBytesToGB(unsigned long long bytes);

	class DAEDALUS_API ResourcesMonitor
	{
	public:
		static float GetAppCPUUsage();
		static float GetTotalCPUUsage();

		static float GetAppRAMUsageInMB();
		static float GetAppRAMUsageInGB();

		static float GetTotalRAMUsageInMB();
		static float GetTotalRAMUsageInGB();

		static float GetSystemRAMInMB();
		static float GetSystemRAMInGB();	
		
		static const std::string& GetProcessorName();

		static const GpuInfo& GetGPUInfo();
	};
}