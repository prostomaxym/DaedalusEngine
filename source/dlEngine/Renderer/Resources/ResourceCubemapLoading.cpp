#include "dlpch.h"
#include "ResourceState.h"
#include "AssetImporter.h"
#include "Core/AssetPipeline.h"
#include <array>

using namespace Daedalus;

namespace
{
	std::string CubemapKey(const std::vector<std::string>& faces)
	{
		std::string key;
		for (const auto& face : faces) key += std::to_string(face.size()) + ":" + face;
		return key;
	}
}

JobHandle ResourceManager::LoadCubemapAsync(const std::vector<std::string>& faces, std::function<void(ResourceLoadResult<TextureCubemap>)> callback)
{
	auto& state = GetState();
	auto* jobs = s_instance->m_jobs;
	if (jobs && !jobs->IsAcceptingJobs()) return jobs->Submit([] {});
	const auto key = CubemapKey(faces);
	auto& entry = state.m_cubemap_sources[key];
	entry.faces = faces;
	const auto generation = ++entry.generation;
	auto* assets = s_instance->m_assets;
	const auto files = assets ? assets->GetFiles() : VirtualFileSystem();
	const auto snapshot = assets ? assets->GetRegistry() : AssetRegistry();
	auto images = std::make_shared<std::vector<std::shared_ptr<DecodedImage>>>();
	auto error = std::make_shared<std::string>();
	if (!assets || faces.size() != 6) *error = "Cubemap import needs six asset faces";
	auto decode = [files, faces, images, error]
	{
		if (!error->empty()) return;
		for (const auto& path : faces)
		{
			auto image = AssetImporter::ImportTexture(files, path, *error, false, 3);
			if (!image) return;
			if (image->GetWidth() != image->GetHeight() || (!images->empty() && image->GetWidth() != images->front()->GetWidth()))
			{ *error = "Cubemap faces must be square and identically sized"; return; }
			images->push_back(std::move(image));
		}
	};
	auto upload = [key, faces, generation, snapshot, images, error, callback = std::move(callback)]() mutable
	{
		auto& current = GetState();
		ResourceLoadResult<TextureCubemap> result;
		if (current.m_cubemap_sources[key].generation != generation) result.error = "Superseded cubemap import";
		if (s_instance->m_assets)
			for (const auto& path : faces)
			{
				const auto* old = snapshot.Find(path);
				const auto* now = s_instance->m_assets->GetRegistry().Find(path);
				if ((old == nullptr) != (now == nullptr) || (old && (old->id != now->id || old->content_hash != now->content_hash))) result.error = "Superseded cubemap source";
			}
		if (result.error.empty()) result.error = *error;
		if (result.error.empty())
		{
			auto replacement = TextureCubemap::Create(*images);
			if (!replacement || !replacement->IsLoaded()) result.error = "Cubemap upload failed";
			else
			{
				if (auto target = current.m_cubemap_sources[key].target.lock())
				{
					if (!target->SwapStorage(*replacement)) result.error = "Incompatible cubemap replacement";
					else replacement = std::move(target);
				}
				if (result.error.empty())
				{
					current.m_unnamed_cubemaps.insert(replacement);
					current.m_cubemap_sources[key].target = replacement;
					result.resource = std::move(replacement);
					++current.m_revision;
				}
			}
		}
		if (callback) callback(std::move(result));
	};
	if (!jobs) { decode(); upload(); return {}; }
	const std::array dependencies{ jobs->Submit(std::move(decode)) };
	return jobs->Submit(std::move(upload), dependencies, JobAffinity::MainThread);
}

std::shared_ptr<TextureCubemap> ResourceManager::LoadCubemapAsset(const std::vector<std::string>& faces)
{
	auto& state = GetState();
	const auto found = state.m_cubemap_sources.find(CubemapKey(faces));
	if (found != state.m_cubemap_sources.end()) if (auto target = found->second.target.lock()) return target;
	ResourceLoadResult<TextureCubemap> result;
	const auto handle = LoadCubemapAsync(faces, [&](auto loaded) { result = std::move(loaded); });
	if (s_instance->m_jobs && !s_instance->m_jobs->Wait(handle)) return nullptr;
	if (!result.Succeeded()) Log::Write(Log::Levels::Error, Log::Categories::Renderer, "{}", result.error);
	return result.resource;
}
