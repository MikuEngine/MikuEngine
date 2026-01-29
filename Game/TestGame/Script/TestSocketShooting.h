#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/GameObject/GameObject.h>

namespace game
{
    class TestSocketShooting :
        public engine::Script<TestSocketShooting>
    {
        REGISTER_SCRIPT(TestSocketShooting, Script)

    public:
        void Awake() override;
        //void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        // 조립에 필요한 오브젝트들
        engine::GameObject* m_player = nullptr;     // SkeletalMesh를 가진 플레이어
        engine::GameObject* m_gun = nullptr;        // 총 오브젝트
        engine::GameObject* m_muzzleFlash = nullptr; // 총구 파티클 오브젝트

        // 에디터에서 선택하기 위한 ID 저장용
        std::string m_playerName = "Player";
        std::string m_gunName = "GunObject";
        std::string m_muzzleFlashName = "ParticleObject";
    };
}