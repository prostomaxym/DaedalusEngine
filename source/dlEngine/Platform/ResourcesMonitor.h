#include "Macros.h"

namespace Daedalus
{
	class DAEDALUS_API ResourcesMonitor
	{
	public:
		static float GetAppCPUUsage();
		static float GetTotalCPUUsage();

		static float GetAppRAMUsageInMB();
		static float GetAppRAMUsageInGB();

		static float GetTotalRAMUsageInMB();
		static float GetTotalRAMUsageInGB();

		static float GetSystemRAMInMB();
		static float GetSystemRAMInGB();	
	};
}