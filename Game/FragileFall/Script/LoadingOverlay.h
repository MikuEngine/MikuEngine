#pragma once

#include <Framework/Object/Component/Script.h>

#include <Framework/Object/Component/UI/UIProgressBar.h>

namespace engine
{

}

namespace game
{
    class LoadingOverlay :
        public engine::Script<LoadingOverlay>
    {
        REGISTER_SCRIPT(LoadingOverlay, Script)

    public:
        //void Awake() override;
        //void Start() override;
        //void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}