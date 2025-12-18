#include "pch.h"
#include "Profiling.h"

#include <psapi.h>
#pragma comment(lib, "psapi.lib")

namespace engine
{
    namespace
    {
        TimePoint g_lastTimestamp = Clock::now();
        int g_lastFPS = 0;
        int g_frameCount = 0;

        UINT64 g_vramUsage = 0;
        UINT64 g_dramUsage = 0;
        UINT64 g_pageFileUsage = 0;
    }

    void Profiling::UpdateFPS(bool print)
    {
        ++g_frameCount;

        if (Time::GetElapsedMilliseconds(g_lastTimestamp) >= 1000LL)
        {
            g_lastTimestamp = Time::GetAccumulatedTimeM(g_lastTimestamp, 1000LL);

            g_lastFPS = g_frameCount;

            g_frameCount = 0;

#ifdef _DEBUG
            if (print)
            {
                LOG_PRINT("FPS: {}", g_lastFPS);
            }
#endif // _DEBUG
        }
    }

    void Profiling::UpdateVRAMUsage(UINT64 usage)
    {
        g_vramUsage = usage;
    }

    void Profiling::UpdateMemoryUsage()
    {
        HANDLE hProcess = GetCurrentProcess();
        PROCESS_MEMORY_COUNTERS_EX pmc{};
        pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);

        GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));

        g_dramUsage = pmc.WorkingSetSize;
        g_pageFileUsage = pmc.PagefileUsage;
    }

    int Profiling::GetLastFPS()
    {
        return g_lastFPS;
    }

    UINT64 Profiling::GetVRAMUsage()
    {
        return g_vramUsage;
    }

    UINT64 Profiling::GetDRAMUsage()
    {
        return g_dramUsage;
    }

    UINT64 Profiling::GetPageFileUsage()
    {
        return g_pageFileUsage;
    }
}
