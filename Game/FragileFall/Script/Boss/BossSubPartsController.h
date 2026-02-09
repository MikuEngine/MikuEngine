#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class BossSubPartsController :
        public engine::Script<BossSubPartsController>
    {
        REGISTER_SCRIPT(BossSubPartsController, Script)

    private:
        float m_accTime = 0.0f; // 누적 시간 변수
        std::vector<engine::Transform*> m_rotatingSubParts;

        int m_orbitPartsCount = 20;
        float m_orbitSpeed = 60.0f;
        float m_orbitRadius = 5.0f;
        float m_bobbingSpeed = 1.5f;
        float m_bobbingAmount = 3.5f;

    public:
        //void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}