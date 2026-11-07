#include <Platform/ResourcesMonitor.h>

#include <gtest/gtest.h>

namespace Daedalus
{
	TEST(ResourcesMonitorTest, zero_bytes_convert_to_zero)
	{
		EXPECT_FLOAT_EQ(ConvertBytesToMB(0), 0.f);
		EXPECT_FLOAT_EQ(ConvertBytesToGB(0), 0.f);
	}

	TEST(ResourcesMonitorTest, binary_megabytes_use_1024_squared_bytes)
	{
		EXPECT_FLOAT_EQ(ConvertBytesToMB(1024ull * 1024ull), 1.f);
		EXPECT_FLOAT_EQ(ConvertBytesToGB(1024ull * 1024ull), 1.f / 1024.f);
	}

	TEST(ResourcesMonitorTest, binary_gigabytes_use_1024_cubed_bytes)
	{
		EXPECT_FLOAT_EQ(ConvertBytesToMB(1024ull * 1024ull * 1024ull), 1024.f);
		EXPECT_FLOAT_EQ(ConvertBytesToGB(1024ull * 1024ull * 1024ull), 1.f);
	}

	TEST(ResourcesMonitorTest, fractional_megabyte_precision_preserves_platform_behavior)
	{
		constexpr auto bytes = 1024ull * 1024ull + 512ull * 1024ull;
#ifdef DL_PLATFORM_ANDROID
		EXPECT_FLOAT_EQ(ConvertBytesToMB(bytes), 1.5f);
		EXPECT_FLOAT_EQ(ConvertBytesToGB(bytes), 1.5f / 1024.f);
#else
		EXPECT_FLOAT_EQ(ConvertBytesToMB(bytes), 1.f);
		EXPECT_FLOAT_EQ(ConvertBytesToGB(bytes), 1.f / 1024.f);
#endif
	}

	TEST(ResourcesMonitorTest, gigabyte_conversion_matches_megabyte_conversion)
	{
		for (const auto bytes : { 0ull, 512ull, 1024ull * 1024ull + 512ull,
			1024ull * 1024ull * 1024ull + 512ull * 1024ull })
			EXPECT_FLOAT_EQ(ConvertBytesToGB(bytes), ConvertBytesToMB(bytes) / 1024.f);
	}

	TEST(ResourcesMonitorTest, large_byte_counts_do_not_truncate_to_32_bits)
	{
		constexpr auto bytes = 1024ull * 1024ull * 1024ull * 1024ull;
		EXPECT_FLOAT_EQ(ConvertBytesToMB(bytes), 1048576.f);
		EXPECT_FLOAT_EQ(ConvertBytesToGB(bytes), 1024.f);
	}

	TEST(ResourcesMonitorTest, default_gpu_info_has_empty_identity_and_zero_measurements)
	{
		const GpuInfo info;
		EXPECT_TRUE(info.name.empty());
		EXPECT_TRUE(info.driver_version.empty());
		EXPECT_EQ(info.temperature_celsius, 0u);
		EXPECT_EQ(info.gpu_utilization_percent, 0u);
		EXPECT_EQ(info.memory_utilization_percent, 0u);
		EXPECT_EQ(info.power_usage_watt, 0u);
		EXPECT_EQ(info.graphics_clock_mhz, 0u);
		EXPECT_EQ(info.memory_clock_mhz, 0u);
		EXPECT_FLOAT_EQ(info.memory_total_GB, 0.f);
		EXPECT_FLOAT_EQ(info.memory_used_GB, 0.f);
	}

	TEST(ResourcesMonitorTest, shared_static_queries_link_without_sampling_hardware)
	{
		using UsageQuery = float (*)();
		// Volatile pointers retain DLL references without calling telemetry or graphics backends.
		UsageQuery volatile usage_queries[]{
			&ResourcesMonitor::GetAppCPUUsage,
			&ResourcesMonitor::GetTotalCPUUsage,
			&ResourcesMonitor::GetAppRAMUsageInMB,
			&ResourcesMonitor::GetAppRAMUsageInGB,
			&ResourcesMonitor::GetTotalRAMUsageInMB,
			&ResourcesMonitor::GetTotalRAMUsageInGB,
			&ResourcesMonitor::GetSystemRAMInMB,
			&ResourcesMonitor::GetSystemRAMInGB
		};
		for (auto& query : usage_queries) EXPECT_NE(query, nullptr);
		auto* volatile processor = &ResourcesMonitor::GetProcessorName;
		auto* volatile device = &ResourcesMonitor::GetDeviceDescription;
		auto* volatile gpu = &ResourcesMonitor::GetGPUInfo;
		EXPECT_NE(processor, nullptr);
		EXPECT_NE(device, nullptr);
		EXPECT_NE(gpu, nullptr);
	}
}
