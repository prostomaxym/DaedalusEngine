#pragma once

#include <cstddef>

namespace Daedalus
{
	template<typename Container, typename Project, typename Release>
	size_t EvictUnused(Container& entries, Project project, Release release)
	{
		size_t removed = 0;
		for (auto it = entries.begin(); it != entries.end();)
		{
			const auto& resource = project(*it);
			if (!resource || resource.use_count() == 1)
			{
				if (resource) release(resource);
				it = entries.erase(it);
				++removed;
			}
			else ++it;
		}
		return removed;
	}
}
