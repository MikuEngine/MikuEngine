#pragma once

namespace game
{
	class TimeScaler
	{
	public:
		static void Initialize();
		static void Update();

		// duration 만큼 world 타임 스케일을 scale 만큼 변경함
		static void ApplyWorldTimeScale(float scale, float duration);

		// duration 만큼 world 타임 스케일을 scale 만큼 변경함
		// duration 중의 start 만큼 선형 보간으로 변화하고 (duration - start - end) 만큼 유지되다가
		// (duration - end) 지점부터 end 만큼 선형 보간으로 변화함
		static void ApplyCrossfadeWorldTimeScale(float scale, float duration, float start, float end);

		// 타임스케일 초기화
		// 각 씬에 빈 게임오브젝트 넣어서 Awake나 Start에서 호출해주면 좋을듯
		static void ResetWorldTimeScale();

		// 타임스케일 조정이 진행 중인지 확인
		static bool IsActive();

		// ui에서 사용할 함수
		static void PlayWorld();
		static void StopWorld();
	};
}