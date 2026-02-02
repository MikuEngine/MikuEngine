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

	private:
		static bool s_has;

		static int s_ruby;
		static int s_sapphire;
		static int s_emerald;

		// 구매한 노드 id 집합(또는 vector/map)
		static std::unordered_set<int> s_purchased;
	};
}