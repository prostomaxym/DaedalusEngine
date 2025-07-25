#include "Macros.h"

namespace Daedalus
{
	class DAEDALUS_API ResourcesMonitor
	{
	public:
		static float GetAppCPUUsage();
		static float GetTotalCPUUsage();
		static float GetAppRAMUsageInMB();
		static float GetTotalRAMUsageInMB();
	};
}