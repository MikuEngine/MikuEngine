#include "GamePCH.h"
#include "GameScene.h"

#include <Framework/Scene/SceneManager.h>
#include <Manager/LoadingScreenDrawer.h>

namespace game
{
    namespace
    {
        // enum class를 unordered_map 키로 안전하게 쓰기 위한 해시
        struct SceneIDHash
        {
            std::size_t operator()(SceneID v) const noexcept
            {
                return static_cast<std::size_t>(v);
            }
        };

        // SceneId -> 실제 씬 이름(파일명/키)
        static const std::unordered_map<SceneID, std::string, SceneIDHash> kSceneNameMap =
        {
            { SceneID::Main,     "01_Ready_Main" },
            { SceneID::Lobby,    "01_Ready_Lobby" },
            { SceneID::Play,     "Stage1_ProtoType" },
            { SceneID::Result,   "00_FIN_Result" },
            { SceneID::Tutorial, "10_PROTO_Tutorial" },
        };

        static const std::string kInvalidSceneName = "INVALID_SCENE";
    }

	const std::string& GameScene::Name(SceneID id)
	{
        auto it = kSceneNameMap.find(id);
        if (it == kSceneNameMap.end())
            return kInvalidSceneName;

        return it->second;
	}

	void GameScene::Change(SceneID id, bool async)
	{
        const std::string& sceneName = Name(id);
        if (sceneName == kInvalidSceneName)
        {
            LOG_ERROR("SceneId mapping missing: {}", sceneName);
            return;
        }

        game::LoadingScreenDrawer::OnSceneTransitionBegin();
        engine::SceneManager::Get().ChangeScene(sceneName, async);
	}
}