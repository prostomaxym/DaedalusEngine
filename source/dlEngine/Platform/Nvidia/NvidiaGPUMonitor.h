#pragma once

#include "Platform/ResourcesMonitor.h"

#include "nvml.h"
#include <string>

namespace Daedalus
{
    using NvmlDeviceHandle = void*;

    class NvidiaGPUMonitor
    {
    public:
        NvidiaGPUMonitor();
        ~NvidiaGPUMonitor();

        const GpuInfo& UpdateInfo();
        static const GpuInfo& GetInfo();
    private:
        HMODULE nvml = nullptr;
        nvmlDevice_t device = nullptr;
        bool initialized = false;
        GpuInfo info;
    };
}