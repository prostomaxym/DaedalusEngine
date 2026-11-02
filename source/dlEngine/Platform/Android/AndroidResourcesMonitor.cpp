#include "Platform/ResourcesMonitor.h"
#include "Platform/OpenGL/OpenGLHeaders.h"
#include <sys/sysinfo.h>
#include <unistd.h>
#include <fstream>
#include <sys/system_properties.h>
#include <time.h>
#include <algorithm>

namespace Daedalus
{
	namespace
	{
		std::string GetProperty(const char* key)
		{
			char value[PROP_VALUE_MAX]{};
			__system_property_get(key, value);
			return value;
		}

		double Seconds(const timespec& value)
		{
			return static_cast<double>(value.tv_sec) + static_cast<double>(value.tv_nsec) / 1e9;
		}
	}

	float ConvertBytesToMB(unsigned long long bytes) { return static_cast<float>(bytes) / (1024.f * 1024.f); }
	float ConvertBytesToGB(unsigned long long bytes) { return ConvertBytesToMB(bytes) / 1024.f; }
	float ResourcesMonitor::GetAppCPUUsage()
	{
		static double previous_cpu = 0.0, previous_wall = 0.0;
		timespec cpu{}, wall{};
		if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpu) != 0 || clock_gettime(CLOCK_MONOTONIC, &wall) != 0)
			return -1.f;
		const auto cpu_seconds = Seconds(cpu);
		const auto wall_seconds = Seconds(wall);
		const auto cpu_delta = cpu_seconds - previous_cpu;
		const auto wall_delta = wall_seconds - previous_wall;
		const bool has_previous = previous_wall > 0.0;
		previous_cpu = cpu_seconds;
		previous_wall = wall_seconds;
		const auto cores = sysconf(_SC_NPROCESSORS_ONLN);
		if (!has_previous || wall_delta <= 0.0 || cpu_delta < 0.0 || cores <= 0)
			return -1.f;
		return static_cast<float>(std::clamp(100.0 * cpu_delta / wall_delta / static_cast<double>(cores), 0.0, 100.0));
	}
	float ResourcesMonitor::GetTotalCPUUsage() { return -1.f; }
	float ResourcesMonitor::GetAppRAMUsageInMB()
	{
		std::ifstream input("/proc/self/statm");
		unsigned long long size = 0, resident = 0;
		const auto page_size = sysconf(_SC_PAGESIZE);
		return input >> size >> resident && page_size > 0 ? ConvertBytesToMB(resident * page_size) : -1.f;
	}
	float ResourcesMonitor::GetAppRAMUsageInGB() { return GetAppRAMUsageInMB() / 1024.f; }
	float ResourcesMonitor::GetTotalRAMUsageInMB()
	{
		struct sysinfo info{};
		return sysinfo(&info) == 0 ? ConvertBytesToMB(static_cast<unsigned long long>(info.totalram - info.freeram) * info.mem_unit) : -1.f;
	}
	float ResourcesMonitor::GetTotalRAMUsageInGB() { return GetTotalRAMUsageInMB() / 1024.f; }
	float ResourcesMonitor::GetSystemRAMInMB()
	{
		struct sysinfo info{};
		return sysinfo(&info) == 0 ? ConvertBytesToMB(static_cast<unsigned long long>(info.totalram) * info.mem_unit) : -1.f;
	}
	float ResourcesMonitor::GetSystemRAMInGB() { return GetSystemRAMInMB() / 1024.f; }
	const std::string& ResourcesMonitor::GetProcessorName()
	{
		static const std::string name = []
		{
			auto model = GetProperty("ro.soc.model");
			if (model.empty()) model = GetProperty("ro.hardware");
			if (model.empty()) model = "Unknown SoC";
			return model + " / " + std::to_string(sysconf(_SC_NPROCESSORS_ONLN)) + " logical CPUs";
		}();
		return name;
	}
	const std::string& ResourcesMonitor::GetDeviceDescription()
	{
		static const std::string description = []
		{
			auto model = GetProperty("ro.product.model");
			if (model.empty()) model = "Unknown device";
			return GetProperty("ro.product.manufacturer") + " " + model + "\nAndroid " +
				GetProperty("ro.build.version.release") + " (API " + GetProperty("ro.build.version.sdk") +
				") / " + GetProperty("ro.product.cpu.abi");
		}();
		return description;
	}
	const GpuInfo& ResourcesMonitor::GetGPUInfo()
	{
		static GpuInfo info;
		if (info.name.empty())
		{
			if (const auto* name = glGetString(GL_RENDERER)) info.name = reinterpret_cast<const char*>(name);
			if (const auto* version = glGetString(GL_VERSION)) info.driver_version = reinterpret_cast<const char*>(version);
		}
		return info;
	}
}
