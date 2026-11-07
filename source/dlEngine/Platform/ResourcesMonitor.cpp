#include "ResourcesMonitor.h"

namespace Daedalus
{
	float ConvertBytesToMB(unsigned long long bytes)
	{
#ifdef DL_PLATFORM_ANDROID
		return static_cast<float>(bytes) / (1024.f * 1024.f);
#else
		return static_cast<float>(bytes / (1024ull * 1024ull));
#endif
	}

	float ConvertBytesToGB(unsigned long long bytes)
	{
		return ConvertBytesToMB(bytes) / 1024.f;
	}
}
