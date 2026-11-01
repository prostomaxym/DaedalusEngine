#pragma once

#include "AssetRegistry.h"
#include "Threads/JobSystem.h"
#include <chrono>
#include <functional>
#include <thread>

namespace Daedalus
{
	enum class AssetLoadMode { Bytes, ShaderSource };

	struct AssetLoadResult
	{
		AssetId id = 0;
		std::string virtual_path;
		std::string data;
		std::string error;
		[[nodiscard]] bool Succeeded() const { return error.empty(); }
	};

	class DAEDALUS_API AssetPipeline
	{
	public:
		using LoadCallback = std::function<void(AssetLoadResult)>;
		using ChangeCallback = std::function<void(const std::vector<std::string>&)>;
		AssetPipeline(JobSystem& jobs, VirtualFileSystem files);
		~AssetPipeline();
		AssetPipeline(const AssetPipeline&) = delete;
		AssetPipeline& operator=(const AssetPipeline&) = delete;
		AssetPipeline(AssetPipeline&&) = delete;
		AssetPipeline& operator=(AssetPipeline&&) = delete;

		[[nodiscard]] bool Initialize();
		[[nodiscard]] const VirtualFileSystem& GetFiles() const { return m_files; }
		[[nodiscard]] const AssetRegistry& GetRegistry() const { return m_registry; }
		[[nodiscard]] JobHandle LoadAsync(AssetId id, LoadCallback callback, AssetLoadMode mode = AssetLoadMode::Bytes);
		[[nodiscard]] JobHandle Refresh();
		void SetChangeCallback(ChangeCallback callback);
		void Update();

	private:
		void CheckOwner() const;
		JobSystem& m_jobs; // Non-owning; Engine owns and drains the scheduler before asset teardown.
		VirtualFileSystem m_files;
		AssetRegistry m_registry;
		ChangeCallback m_on_change;
		JobHandle m_scan;
		std::chrono::steady_clock::time_point m_next_poll{};
		std::thread::id m_owner = std::this_thread::get_id();
	};
}
