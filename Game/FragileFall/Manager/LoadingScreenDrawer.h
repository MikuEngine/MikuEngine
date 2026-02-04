#pragma once

namespace game
{
	class LoadingScreenDrawer
	{
	public:
		static void Draw(float progress);

		/// 첫 씬 로딩이 끝났을 때 게임에서 한 번 호출 (씬 전환 로딩 화면으로 전환됨)
		static void OnFirstLoadFinished();

		// 씬 로딩 시작시 값 초기화용
		static void OnSceneTransitionBegin();

		/// 앱 셧다운 시 호출 (로딩 화면 리소스 해제)
		static void OnShutdown();
	};
}
