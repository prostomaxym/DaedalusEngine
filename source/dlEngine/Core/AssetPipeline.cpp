#include "dlpch.h"
#include "AssetPipeline.h"
#include "Utils/AssetSourceReader.h"
#include <array>

namespace Daedalus
{
	AssetPipeline::AssetPipeline(JobSystem& jobs, VirtualFileSystem files) : m_jobs(jobs), m_files(std::move(files)) {}
	AssetPipeline::~AssetPipeline() { m_jobs.WaitIdle(); }

	void AssetPipeline::CheckOwner() const
	{
		DL_ASSERT(m_owner == std::this_thread::get_id(), Log::Categories::EngineCore, "AssetPipeline must be accessed on its owner thread");
		if (m_owner != std::this_thread::get_id()) std::abort();
	}

	bool AssetPipeline::Initialize()
	{
		CheckOwner();
		if (!m_scan.IsComplete()) return false;
		return m_registry.Scan(m_files);
	}

	JobHandle AssetPipeline::LoadAsync(AssetId id, LoadCallback callback, AssetLoadMode mode)
	{
		CheckOwner();
		const auto* metadata = m_registry.Find(id);
		auto result = std::make_shared<AssetLoadResult>();
		result->id = id;
		if (metadata) result->virtual_path = metadata->virtual_path;
		const auto read = m_jobs.Submit([files = m_files, result, mode]
		{
			const auto path = files.Resolve(result->virtual_path);
			if (!path) { result->error = "Unknown asset UUID or invalid virtual path"; return; }
			if (mode == AssetLoadMode::ShaderSource)
			{
				(void)AssetSourceReader::ReadShaderSource(files, result->virtual_path, result->data, result->error);
			}
			else (void)AssetSourceReader::ReadBytes(*path, result->data, result->error);
			if (!result->error.empty()) result->data.clear();
		});
		const std::array dependencies{ read };
		return m_jobs.Submit([result, callback = std::move(callback)]() mutable
		{
			if (callback) callback(std::move(*result));
		}, dependencies, JobAffinity::MainThread);
	}

	JobHandle AssetPipeline::Refresh()
	{
		CheckOwner();
		if (!m_scan.IsComplete()) return m_scan;
		auto registry = std::make_shared<AssetRegistry>();
		const auto scan = m_jobs.Submit([files = m_files, registry] { (void)registry->Scan(files); });
		const std::array dependencies{ scan };
		m_scan = m_jobs.Submit([this, registry]
		{
			std::vector<std::string> changed;
			for (const auto& [path, metadata] : registry->GetAssets())
			{
				const auto* previous = m_registry.Find(path);
				if (!previous || previous->id != metadata.id || previous->write_time != metadata.write_time || previous->size != metadata.size)
					changed.push_back(path);
			}
			for (const auto& [path, metadata] : m_registry.GetAssets())
				if (!registry->Find(path)) changed.push_back(path);
			if (registry->GetErrors() != m_registry.GetErrors())
				for (const auto& error : registry->GetErrors())
					Log::Write(Log::Levels::Error, Log::Categories::EngineCore, "Asset registry: {}", error);
			m_registry = std::move(*registry);
			if (m_on_change && !changed.empty()) m_on_change(changed);
		}, dependencies, JobAffinity::MainThread);
		return m_scan;
	}

	void AssetPipeline::SetChangeCallback(ChangeCallback callback)
	{
		CheckOwner();
		m_on_change = std::move(callback);
	}

	void AssetPipeline::Update()
	{
		CheckOwner();
		const auto now = std::chrono::steady_clock::now();
		if (now < m_next_poll || !m_scan.IsComplete()) return;
		m_next_poll = now + std::chrono::seconds(1);
		(void)Refresh();
	}
}
