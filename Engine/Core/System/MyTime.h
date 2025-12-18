#pragma once

namespace engine
{
    class Time
    {
    public:
        static void Update();

        static float DeltaTime(size_t scaleSlot = 0);
        static float FixedDeltaTime();
        static float UnscaledDeltaTime();

        static void SetTimeScale(size_t scaleSlot, float timeScale);
        static void SetFixedDeltaTime(float fixedDeltaTime);

        static TimePoint GetTimestamp();
        static TimePoint GetAccumulatedTimeS(const TimePoint& timePoint, int seconds);
        static TimePoint GetAccumulatedTimeM(const TimePoint& timePoint, long long milliseconds);
        static float GetElapsedSeconds(const TimePoint& timePoint);
        static long long GetElapsedMilliseconds(const TimePoint& timePoint);
    };
}
