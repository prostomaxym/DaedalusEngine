#include <Debug/Profiler.h>
#include <Debug/ProfilerStats.h>

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <thread>

using Daedalus::ProfileFrame;
using Daedalus::ProfileSample;
using Daedalus::ProfileScope;
using Daedalus::Profiler;

class ProfilerTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		Profiler::Reset();
		Profiler::SetEnabled(true);
	}

	void TearDown() override
	{
		Profiler::Reset();
		Profiler::SetEnabled(true);
	}
};

TEST_F(ProfilerTest, scope_records_sample_in_completed_frame)
{
	Profiler::BeginFrame();
	{
		ProfileScope scope("Outer");
	}
	Profiler::EndFrame();

	ProfileFrame frame;
	Profiler::CopyLastFrame(frame);
	ASSERT_EQ(frame.samples.size(), 1u);
	EXPECT_STREQ(frame.samples[0].name, "Outer");
	EXPECT_GE(frame.samples[0].duration_us, 0);
	EXPECT_EQ(frame.samples[0].depth, 0u);
	EXPECT_GE(frame.duration_us, frame.samples[0].duration_us);
}

TEST_F(ProfilerTest, nested_scopes_track_depth)
{
	Profiler::BeginFrame();
	{
		ProfileScope outer("Outer");
		{
			ProfileScope inner("Inner");
		}
	}
	Profiler::EndFrame();

	ProfileFrame frame;
	Profiler::CopyLastFrame(frame);
	ASSERT_EQ(frame.samples.size(), 2u);
	EXPECT_STREQ(frame.samples[0].name, "Inner");
	EXPECT_EQ(frame.samples[0].depth, 1u);
	EXPECT_STREQ(frame.samples[1].name, "Outer");
	EXPECT_EQ(frame.samples[1].depth, 0u);
	EXPECT_LE(frame.samples[1].start_us, frame.samples[0].start_us);
}

TEST_F(ProfilerTest, disabled_profiler_records_nothing)
{
	Profiler::SetEnabled(false);
	Profiler::BeginFrame();
	{
		ProfileScope scope("Ignored");
	}
	Profiler::EndFrame();

	ProfileFrame frame;
	Profiler::CopyLastFrame(frame);
	EXPECT_TRUE(frame.samples.empty());
}

TEST_F(ProfilerTest, end_frame_starts_new_empty_frame)
{
	Profiler::BeginFrame();
	{
		ProfileScope scope("First");
	}
	Profiler::EndFrame();
	Profiler::BeginFrame();
	Profiler::EndFrame();

	ProfileFrame frame;
	Profiler::CopyLastFrame(frame);
	EXPECT_TRUE(frame.samples.empty());
	EXPECT_EQ(frame.index, 1u);
}

TEST_F(ProfilerTest, samples_beyond_capacity_are_dropped)
{
	Profiler::BeginFrame();
	for (size_t i = 0; i < Profiler::MaxSamplesPerFrame + 5; ++i)
		Profiler::Submit({ "S", 0, 1, 0, 0 });
	Profiler::EndFrame();

	ProfileFrame frame;
	Profiler::CopyLastFrame(frame);
	EXPECT_EQ(frame.samples.size(), Profiler::MaxSamplesPerFrame);
	EXPECT_EQ(frame.dropped_samples, 5u);
}

TEST_F(ProfilerTest, worker_thread_samples_have_distinct_thread_id)
{
	Profiler::BeginFrame();
	{
		ProfileScope scope("Main");
	}
	std::thread worker([] { ProfileScope scope("Worker"); });
	worker.join();
	Profiler::EndFrame();

	ProfileFrame frame;
	Profiler::CopyLastFrame(frame);
	ASSERT_EQ(frame.samples.size(), 2u);
	EXPECT_NE(frame.samples[0].thread_id, frame.samples[1].thread_id);
	EXPECT_EQ(frame.samples[1].depth, 0u);
}

TEST_F(ProfilerTest, chrome_trace_contains_complete_events)
{
	std::ostringstream stream;
	Profiler::WriteChromeTrace(stream, { ProfileSample{ "A\"b", 10, 20, 3, 0 } });

	const auto json = stream.str();
	EXPECT_EQ(json.find("{\"displayTimeUnit\":\"ms\",\"traceEvents\":["), 0u);
	EXPECT_NE(json.find("\"name\":\"A\\\"b\""), std::string::npos);
	EXPECT_NE(json.find("\"ph\":\"X\",\"ts\":10,\"dur\":20,\"pid\":1,\"tid\":3"), std::string::npos);
	EXPECT_EQ(json.back(), '}');
}

TEST_F(ProfilerTest, chrome_trace_of_empty_capture_is_valid)
{
	std::ostringstream stream;
	Profiler::WriteChromeTrace(stream, {});
	EXPECT_EQ(stream.str(), "{\"displayTimeUnit\":\"ms\",\"traceEvents\":[]}");
}

TEST_F(ProfilerTest, capture_records_frames_and_samples)
{
	Profiler::StartCapture();
	EXPECT_TRUE(Profiler::IsCapturing());
	Profiler::BeginFrame();
	{
		ProfileScope scope("Captured");
	}
	Profiler::EndFrame();

	// Frame marker + one sample
	EXPECT_EQ(Profiler::GetCaptureSampleCount(), 2u);
}

TEST_F(ProfilerTest, stop_capture_without_start_fails)
{
	EXPECT_FALSE(Profiler::StopCapture(std::filesystem::temp_directory_path() / "dl_profiler_unused.json"));
}

TEST_F(ProfilerTest, stop_capture_writes_file)
{
	const auto file = std::filesystem::temp_directory_path() / "dl_profiler_test_trace.json";
	std::filesystem::remove(file);

	Profiler::StartCapture();
	Profiler::BeginFrame();
	Profiler::EndFrame();
	ASSERT_TRUE(Profiler::StopCapture(file));
	EXPECT_FALSE(Profiler::IsCapturing());
	EXPECT_TRUE(std::filesystem::exists(file));
	EXPECT_EQ(Profiler::GetCaptureSampleCount(), 0u);

	std::filesystem::remove(file);
}

TEST_F(ProfilerTest, gpu_samples_are_copied_with_gpu_thread_id)
{
	const ProfileSample gpu[2] = { { "PassA", 10, 5, 0, 0 }, { "PassB", 20, 7, 0, 0 } };
	Profiler::SubmitGpuSamples(gpu, 2);

	ProfileFrame frame;
	Profiler::CopyLastFrame(frame);
	ASSERT_EQ(frame.gpu_samples.size(), 2u);
	EXPECT_STREQ(frame.gpu_samples[1].name, "PassB");
	EXPECT_EQ(frame.gpu_samples[1].duration_us, 7);
	EXPECT_EQ(frame.gpu_samples[0].thread_id, Profiler::GpuThreadId);
}

TEST_F(ProfilerTest, gpu_samples_are_added_to_capture)
{
	Profiler::StartCapture();
	const ProfileSample gpu{ "Pass", 0, 1, 0, 0 };
	Profiler::SubmitGpuSamples(&gpu, 1);
	EXPECT_EQ(Profiler::GetCaptureSampleCount(), 1u);
	ASSERT_TRUE(Profiler::StopCapture(std::filesystem::temp_directory_path() / "dl_profiler_gpu_trace.json"));
	std::filesystem::remove(std::filesystem::temp_directory_path() / "dl_profiler_gpu_trace.json");
}

TEST(ProfilerStatsTest, aggregates_calls_and_rolling_average)
{
	Daedalus::ProfilerStats stats;
	std::vector<ProfileSample> frame1 = { { "A", 0, 1000, 0, 0 }, { "A", 0, 1000, 0, 0 } };
	std::vector<ProfileSample> frame2 = { { "A", 0, 4000, 0, 0 } };
	stats.AddFrame(frame1);
	stats.AddFrame(frame2);

	const auto* entry = stats.Find("A");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->last_calls, 1u);
	EXPECT_FLOAT_EQ(entry->last_ms, 4.f);
	EXPECT_FLOAT_EQ(entry->avg_ms, 3.f);
	EXPECT_FLOAT_EQ(entry->max_ms, 4.f);

	float history[Daedalus::ProfilerStats::HistorySize];
	ASSERT_EQ(entry->CopyHistory(history), 2u);
	EXPECT_FLOAT_EQ(history[0], 2.f);
	EXPECT_FLOAT_EQ(history[1], 4.f);
}

TEST(ProfilerStatsTest, missing_scope_records_zero_and_history_wraps)
{
	Daedalus::ProfilerStats stats;
	stats.AddFrame({ { "A", 0, 1000, 0, 0 } });
	stats.AddFrame({});
	EXPECT_FLOAT_EQ(stats.Find("A")->last_ms, 0.f);
	EXPECT_EQ(stats.Find("A")->last_calls, 0u);

	for (size_t i = 0; i < Daedalus::ProfilerStats::HistorySize * 2; ++i)
		stats.AddFrame({ { "A", 0, 2000, 0, 0 } });
	EXPECT_EQ(stats.Find("A")->history_count, Daedalus::ProfilerStats::HistorySize);
	EXPECT_FLOAT_EQ(stats.Find("A")->avg_ms, 2.f);
	EXPECT_EQ(stats.Find("B"), nullptr);
}

TEST(ProfilerStatsTest, percentile_uses_nearest_rank)
{
	float values[100];
	for (int i = 0; i < 100; ++i)
		values[i] = static_cast<float>(100 - i);
	EXPECT_FLOAT_EQ(Daedalus::ProfilerStats::Percentile(values, 100, 0.95f), 95.f);
	EXPECT_FLOAT_EQ(Daedalus::ProfilerStats::Percentile(values, 100, 0.99f), 99.f);
	EXPECT_FLOAT_EQ(Daedalus::ProfilerStats::Percentile(values, 100, 1.f), 100.f);
	EXPECT_FLOAT_EQ(Daedalus::ProfilerStats::Percentile(values, 0, 0.5f), 0.f);
}

TEST(ProfilerStatsTest, entry_tracks_percentiles)
{
	Daedalus::ProfilerStats stats;
	for (int i = 1; i <= 100; ++i)
		stats.AddFrame({ { "A", 0, i * 1000, 0, 0 } });
	EXPECT_FLOAT_EQ(stats.Find("A")->p95_ms, 95.f);
	EXPECT_FLOAT_EQ(stats.Find("A")->p99_ms, 99.f);
}

TEST_F(ProfilerTest, stats_update_every_frame_without_ui)
{
	for (int i = 0; i < 3; ++i)
	{
		Profiler::BeginFrame();
		{
			ProfileScope scope("Work");
		}
		Profiler::EndFrame();
	}
	const ProfileSample gpu{ "GpuPass", 0, 2000, 0, 0 };
	Profiler::SubmitGpuSamples(&gpu, 1);

	std::vector<Daedalus::ProfilerStatsEntry> cpu;
	std::vector<Daedalus::ProfilerStatsEntry> gpu_stats;
	Profiler::CopyStats(cpu, gpu_stats);
	ASSERT_EQ(cpu.size(), 1u);
	EXPECT_STREQ(cpu[0].name, "Work");
	EXPECT_EQ(cpu[0].history_count, 3u);
	ASSERT_EQ(gpu_stats.size(), 1u);
	EXPECT_FLOAT_EQ(gpu_stats[0].last_ms, 2.f);
}

TEST_F(ProfilerTest, dropped_gpu_frames_are_counted_and_reset)
{
	EXPECT_EQ(Profiler::GetDroppedGpuFrames(), 0u);
	Profiler::ReportDroppedGpuFrame();
	Profiler::ReportDroppedGpuFrame();
	EXPECT_EQ(Profiler::GetDroppedGpuFrames(), 2u);
	Profiler::Reset();
	EXPECT_EQ(Profiler::GetDroppedGpuFrames(), 0u);
}
