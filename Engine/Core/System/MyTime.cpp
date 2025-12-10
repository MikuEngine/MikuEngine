#include "pch.h"
#include "MyTime.h"

namespace engine
{
	namespace
	{
		TimePoint g_previousTime = Clock::now();
		TimePoint g_currentTime = Clock::now();
		float g_deltaTime = 0.0f;
		float g_fixedDeltaTime = 0.02f;
		std::vector<float> g_timeScales{ 1.0f };
	}

	void Time::Update()
	{
		g_currentTime = Clock::now();

		g_deltaTime = std::chrono::duration<float>(g_currentTime - g_previousTime).count();

		g_previousTime = g_currentTime;
	}

	float Time::DeltaTime(size_t scaleSlot)
	{
		return g_deltaTime * g_timeScales[scaleSlot];
	}

	float Time::FixedDeltaTime()
	{
		return g_fixedDeltaTime;
	}

	float Time::UnscaledDeltaTime()
	{
		return g_deltaTime;
	}

	void Time::SetTimeScale(size_t scaleSlot, float timeScale)
	{
		if (g_timeScales.size() - 1 < scaleSlot)
		{
			g_timeScales.resize(scaleSlot);
		}

		g_timeScales[scaleSlot] = timeScale;
	}

	void Time::SetFixedDeltaTime(float fixedDeltaTime)
	{
		g_fixedDeltaTime = fixedDeltaTime;
	}

	TimePoint Time::GetTimestamp()
	{
		return Clock::now();
	}

	TimePoint Time::GetAccumulatedTimeS(const TimePoint& timePoint, int seconds)
	{
		return timePoint + std::chrono::seconds(seconds);
	}

	TimePoint Time::GetAccumulatedTimeM(const TimePoint& timePoint, long long milliseconds)
	{
		return timePoint + std::chrono::milliseconds(milliseconds);
	}

	float Time::GetElapsedSeconds(const TimePoint& timePoint)
	{
		return std::chrono::duration<float>(Clock::now() - timePoint).count();
	}

	long long Time::GetElapsedMilliseconds(const TimePoint& timePoint)
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - timePoint).count();
	}
}