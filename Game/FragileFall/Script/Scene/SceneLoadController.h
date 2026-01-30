#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/UI/UIProgressBar.h>

namespace game
{
    class SceneLoadController :
        public engine::Script<SceneLoadController>
    {
        REGISTER_SCRIPT(SceneLoadController, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void RequestChangeScene(const std::string& sceneName);

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void CacheRefs();

    private:
        std::string m_overlayObjectName = "UI_LoadingOverlay";

        engine::GameObject* m_overlayGO = nullptr;
        engine::UIProgressBar* m_bar = nullptr;

        bool m_prevLoading = false;
    };
}