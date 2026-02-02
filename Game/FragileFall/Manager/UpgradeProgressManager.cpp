#include "GamePCH.h"
#include "UpgradeProgressManager.h"
#include <Script/UpgradeController.h>

namespace game
{
	bool UpgradeProgressManager::s_has = false;
	int  UpgradeProgressManager::s_ruby = 0;
	int  UpgradeProgressManager::s_sapphire = 0;
	int  UpgradeProgressManager::s_emerald = 0;
	std::unordered_set<int> UpgradeProgressManager::s_purchased;

	void UpgradeProgressManager::Reset()
	{
		s_has = false;
		s_ruby = s_sapphire = s_emerald = 0;
		s_purchased.clear();
		LOG_PRINT("[UpgradeProgress] Reset");
	}

	void UpgradeProgressManager::SaveProgress(const UpgradeController& uc)
	{
		s_ruby = uc.GetRuby();
		s_sapphire = uc.GetSapphire();
		s_emerald = uc.GetEmerald();

		s_purchased.clear();
		uc.ForEachPurchasedTrue([&](int id)
			{
				s_purchased.insert(id);
			});

		s_has = true;
	}

	bool UpgradeProgressManager::LoadProgress(UpgradeController& uc)
	{
		if (!s_has) return false;

		uc.SetCurrency(s_ruby, s_sapphire, s_emerald);
		uc.SetPurchasedFromSet(s_purchased);

		uc.RecomputeUnlocked();
		uc.RefreshNodeVisuals();

		return true;
	}

	bool UpgradeProgressManager::HasProgress()
	{
		return s_has;
	}
}