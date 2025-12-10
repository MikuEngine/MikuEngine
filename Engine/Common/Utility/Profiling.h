#pragma once

namespace engine
{
	class Profiling
	{
	public:
		static void UpdateFPS(bool print);
		static void UpdateVRAMUsage(UINT64 usage);
		static void UpdateMemoryUsage();

		static int GetLastFPS();
		static UINT64 GetVRAMUsage();
		static UINT64 GetDRAMUsage();
		static UINT64 GEtPageFileUsage();
	};
}
