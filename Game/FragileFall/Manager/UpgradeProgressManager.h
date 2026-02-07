#pragma once
#include <unordered_set>
#include <cstdint>

namespace game
{
	class UpgradeController;

	class UpgradeProgressManager
	{
	public:
		static void Reset();

		static void SaveProgress(const UpgradeController& uc);
		static bool LoadProgress(UpgradeController& uc);

		static bool HasProgress();

		/// 플레이 런에서 번 재화를 정적 저장소에 더함. (복귀 시 StageManager에서 호출)
		static void AddCurrency(int ruby, int sapphire, int emerald);

	private:
		static bool s_has;

		static int s_ruby;
		static int s_sapphire;
		static int s_emerald;

		// 구매한 노드 id 집합(또는 vector/map)
		static std::unordered_set<int> s_purchased;
	};
}