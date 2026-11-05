#pragma once

#include "Macros.h"
#include <cstddef>
#include <string>

namespace Daedalus
{
	struct SceneRecord;
	struct PrefabRecord;
	class VirtualFileSystem;

	class DAEDALUS_API SceneComposer
	{
	public:
		static constexpr size_t MaxPrefabDepth = 16;
		static constexpr size_t MaxPrefabInstances = 1024;
		static constexpr size_t MaxEntities = 100000;
		static constexpr size_t MaxPrefabBytes = 64 * 1024 * 1024;

		// Expands records only; failure leaves the destination unchanged.
		static bool Expand(const SceneRecord& source, const VirtualFileSystem& files, SceneRecord& record, std::string& error);
		// Builds one anchor (UUID 1); a template camera is optional for spawning.
		static bool ExpandPrefab(const PrefabRecord& prefab, const VirtualFileSystem& files, SceneRecord& record, std::string& error);
	};
}
