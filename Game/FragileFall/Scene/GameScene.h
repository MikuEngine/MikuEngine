#pragma once
#include "Scene/SceneId.h"

namespace game
{
	class GameScene
	{
	public:
		static const std::string& Name(SceneID id);
		static void Change(SceneID id, bool async = true);
	};
}