#include "GamePCH.h"
#include "TimeScaler.h"

namespace game
{
	namespace
	{
		constexpr size_t WORLD = 0;
		constexpr size_t SCALER_TIME = 1;

		// 상태 관리 변수
		float g_timer = 0.0f;
		float g_duration = -1.0f;

		// Crossfade용 변수
		bool g_isCrossfading = false;

		float g_startScale = 1.0f;  // 효과 시작 시점의 스케일 (Capture용)
		float g_targetScale = 1.0f; // 목표 스케일

		float g_fadeStart = 0.0f;   // 진입 시간 (Attack)
		float g_fadeEnd = 0.0f;     // 복구 시간 (Release)

		bool g_worldStopped = false;

		float Lerp(float a, float b, float t)
		{
			return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
		}
	}

	void TimeScaler::Initialize()
	{
		engine::Time::SetTimeScale(SCALER_TIME, 1.0f);
	}

	void TimeScaler::Update()
	{
		if (g_worldStopped || g_duration < 0.0f)
		{
			return;
		}

		// 스케일러 자체의 시간 흐름 (월드 스케일 영향 X)
		g_timer += engine::Time::DeltaTime(SCALER_TIME);

		if (g_timer >= g_duration)
		{
			ResetWorldTimeScale();
			return;
		}

		// Crossfade 로직 (ASR Envelope)
		if (g_isCrossfading)
		{
			float currentScale = 1.0f;

			if (g_timer < g_fadeStart) // 진입
			{
				float alpha = (g_fadeStart > 0.0f) ? (g_timer / g_fadeStart) : 1.0f;
				// 캡처해둔 g_startScale에서 시작함
				currentScale = Lerp(g_startScale, g_targetScale, alpha);
			}
			else if (g_timer > (g_duration - g_fadeEnd)) // 복구
			{
				float fadeOutStartTime = g_duration - g_fadeEnd;
				float timeInFadeOut = g_timer - fadeOutStartTime;
				float alpha = (g_fadeEnd > 0.0f) ? (timeInFadeOut / g_fadeEnd) : 1.0f;

				// 끝나면 다시 원래 속도(1.0f)로 돌아감
				currentScale = Lerp(g_targetScale, 1.0f, alpha);
			}
			else // 유지
			{
				currentScale = g_targetScale;
			}

			engine::Time::SetTimeScale(WORLD, currentScale);
		}
	}

	void TimeScaler::ApplyWorldTimeScale(float scale, float duration)
	{
		g_timer = 0.0f;
		g_duration = duration;

		g_isCrossfading = false;
		engine::Time::SetTimeScale(WORLD, scale);
	}

	void TimeScaler::ApplyCrossfadeWorldTimeScale(float scale, float duration, float start, float end)
	{
		g_timer = 0.0f;
		g_duration = duration;

		// Crossfade 설정
		g_isCrossfading = true;
		g_targetScale = scale;
		g_fadeStart = start;
		g_fadeEnd = end;

		// 현재 월드의 시간 스케일을 가져와서 시작점으로 설정
		g_startScale = engine::Time::GetTimeScale(WORLD);

		// 첫 프레임 적용 (start가 0이면 즉시 target, 아니면 현재 속도 유지하며 시작)
		if (start <= 0.0f)
		{
			engine::Time::SetTimeScale(WORLD, scale);
		}
	}

	void TimeScaler::ResetWorldTimeScale()
	{
		engine::Time::SetTimeScale(WORLD, 1.0f);
		g_duration = -1.0f;
		g_isCrossfading = false;
	}

	bool TimeScaler::IsActive()
	{
		return g_duration >= 0.0f && !g_worldStopped;
	}

	void TimeScaler::PlayWorld()
	{
		engine::Time::SetTimeScale(WORLD, 1.0f);
		g_worldStopped = false;
		g_isCrossfading = false;
	}

	void TimeScaler::StopWorld()
	{
		engine::Time::SetTimeScale(WORLD, 0.0f);
		g_worldStopped = true;
	}
}