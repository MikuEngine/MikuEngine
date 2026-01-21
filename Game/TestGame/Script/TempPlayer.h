#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class AimPointer;
    class TempBulletFactory;

    class TempPlayer :
        public engine::Script<TempPlayer>
    {
        REGISTER_COMPONENT(TempPlayer)

    private:
        float m_moveSpeed = 5.0f;
        
        // 씬에서 이름으로 찾을 오브젝트들
        AimPointer* m_aimPointer = nullptr;
        TempBulletFactory* m_bulletFactory = nullptr;

    public:
        void Start() override;
        void Update() override;

    private:
        void HandleMovement();
        void HandleShooting();

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;
    };
}
