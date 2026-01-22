#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class ButtonCallbackTest2 :
        public engine::Script<ButtonCallbackTest2>
    {
        REGISTER_COMPONENT(ButtonCallbackTest2, Script)

    public:
        void Awake() override;
        //void Start() override;
        //void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void CallThat();
    };
}