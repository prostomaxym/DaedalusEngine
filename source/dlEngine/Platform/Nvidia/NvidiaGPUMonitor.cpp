#include "dlpch.h"
#include "NVIDIAGPUMonitor.h"

#ifdef DL_PLATFORM_WINDOWS
#include <windows.h>
#elif DL_PLATFORM_LINUX
#include <dlfcn.h>
#endif

// Function pointer declarations (types from nvml.h)
nvmlReturn_t(*pnvmlInit_v2)();
nvmlReturn_t(*pnvmlShutdown)();
nvmlReturn_t(*pnvmlDeviceGetHandleByIndex)(unsigned int, nvmlDevice_t*);
nvmlReturn_t(*pnvmlDeviceGetName)(nvmlDevice_t, char*, unsigned int);
nvmlReturn_t(*pnvmlSystemGetDriverVersion)(char*, unsigned int);
nvmlReturn_t(*pnvmlDeviceGetUtilizationRates)(nvmlDevice_t, nvmlUtilization_t*);
nvmlReturn_t(*pnvmlDeviceGetTemperature)(nvmlDevice_t, nvmlTemperatureSensors_t, unsigned int*);
nvmlReturn_t(*pnvmlDeviceGetPowerUsage)(nvmlDevice_t, unsigned int*);
nvmlReturn_t(*pnvmlDeviceGetClockInfo)(nvmlDevice_t, nvmlClockType_t, unsigned int*);
nvmlReturn_t(*pnvmlDeviceGetMemoryInfo)(nvmlDevice_t, nvmlMemory_t*);

#ifdef DL_PLATFORM_WINDOWS
#define LOAD_FUNC(name) \
        p##name = reinterpret_cast<decltype(p##name)>(GetProcAddress(nvml, #name)); \
        if (!p##name) { initialized = false; return; }
#elif DL_PLATFORM_LINUX
#define LOAD_FUNC(name) \
        p##name = reinterpret_cast<decltype(p##name)>(dlsym(nvml, #name)); \
        if (!p##name) { initialized = false; return; }
#endif

using namespace Daedalus;

NvidiaGPUMonitor::NvidiaGPUMonitor()
{
#ifdef DL_PLATFORM_WINDOWS
    nvml = LoadLibraryA("nvml.dll");
#elif DL_PLATFORM_LINUX
    nvml = dlopen("libnvidia-ml.so.1", RTLD_LAZY);
#endif

    if (!nvml)
    {
        initialized = false;
        return;
    }

    LOAD_FUNC(nvmlInit_v2);
    LOAD_FUNC(nvmlShutdown);
    LOAD_FUNC(nvmlDeviceGetHandleByIndex);
    LOAD_FUNC(nvmlDeviceGetName);
    LOAD_FUNC(nvmlSystemGetDriverVersion);
    LOAD_FUNC(nvmlDeviceGetUtilizationRates);
    LOAD_FUNC(nvmlDeviceGetTemperature);
    LOAD_FUNC(nvmlDeviceGetPowerUsage);
    LOAD_FUNC(nvmlDeviceGetClockInfo);
    LOAD_FUNC(nvmlDeviceGetMemoryInfo);

#undef LOAD_FUNC

    pnvmlInit_v2();

    const auto result = pnvmlDeviceGetHandleByIndex(0, &device);
    if (result != NVML_SUCCESS)
    {
        initialized = false;
        return;
    }

    initialized = true;

    char name_buf[NVML_DEVICE_NAME_BUFFER_SIZE];
    char driver_buf[NVML_SYSTEM_DRIVER_VERSION_BUFFER_SIZE];

    pnvmlDeviceGetName(device, name_buf, sizeof(name_buf));
    pnvmlSystemGetDriverVersion(driver_buf, sizeof(driver_buf));

    info.name = name_buf;
    info.driver_version = driver_buf;
}

NvidiaGPUMonitor::~NvidiaGPUMonitor()
{
    if (initialized)
    {
        pnvmlShutdown();
#ifdef DL_PLATFORM_WINDOWS
        FreeLibrary(nvml);
#elif DL_PLATFORM_LINUX
        dlclose(nvml);
#endif
    }
}

const GpuInfo& NvidiaGPUMonitor::UpdateInfo()
{
    if (!initialized)
        return info;

    nvmlUtilization_t util;
    pnvmlDeviceGetUtilizationRates(device, &util);
    info.gpu_utilization_percent = util.gpu;
    info.memory_utilization_percent = util.memory;

    pnvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &info.temperature_celsius);
    unsigned int usage_mw = 0;
    pnvmlDeviceGetPowerUsage(device, &usage_mw);
    info.power_usage_watt = static_cast<float>(usage_mw / 1000.f);
    pnvmlDeviceGetClockInfo(device, NVML_CLOCK_GRAPHICS, &info.graphics_clock_mhz);
    pnvmlDeviceGetClockInfo(device, NVML_CLOCK_MEM, &info.memory_clock_mhz);

    nvmlMemory_t mem;
    pnvmlDeviceGetMemoryInfo(device, &mem);
    info.memory_total_GB = ConvertBytesToGB(mem.total);
    info.memory_used_GB = ConvertBytesToGB(mem.used);

    return info;
}

namespace
{
    NvidiaGPUMonitor monitor;
}

const GpuInfo& NvidiaGPUMonitor::GetInfo()
{
    return monitor.UpdateInfo();
}