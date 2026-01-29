#pragma once

namespace game
{
	enum class ItemType : int
	{
		None = -1,
		Ruby = 0,
		Sapphire,
		Emerald
	};

	struct ItemCost
	{
		ItemType type = ItemType::None;
		int amount = 0;
	};
}