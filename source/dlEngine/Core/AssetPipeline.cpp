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
		return LoadPath(id, metadata ? metadata->virtual_path : std::string(), std::move(callback), mode);
	}

	JobHandle AssetPipeline::LoadAsync(std::string_view path, LoadCallback callback, AssetLoadMode mode)
	{
		CheckOwner();
		const auto normalized = VirtualFileSystem::Normalize(path);
		const auto* metadata = normalized ? m_registry.Find(*normalized) : nullptr;
		return LoadPath(metadata ? metadata->id : 0, normalized.value_or(std::string()), std::move(callback), mode);
	}

	JobHandle AssetPipeline::LoadPath(AssetId id, std::string path, LoadCallback callback, AssetLoadMode mode)
	{
		auto result = std::make_shared<AssetLoadResult>();
		result->id = id;
		result->virtual_path = std::move(path);
		const auto read = m_jobs.Submit([files = m_files, result, mode]
		{
			if (result->virtual_path.empty()) { result->error = "Unknown asset UUID"; return; }
			if (mode == AssetLoadMode::ShaderSource)
			{
				(void)AssetSourceReader::ReadShaderSource(files, result->virtual_path, result->data, result->error);
			}
			else (void)files.Read(result->virtual_path, result->data, result->error);
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
				if (!previous || previous->id != metadata.id || previous->write_time != metadata.write_time || previous->size != metadata.size || previous->content_hash != metadata.content_hash)
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
