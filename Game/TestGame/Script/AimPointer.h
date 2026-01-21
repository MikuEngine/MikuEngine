#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class AimPointer :
        public engine::Script<AimPointer>
    {
        REGISTER_COMPONENT(AimPointer)

    private:
        engine::Vector3 m_worldPosition;  // 마우스의 월드 좌표

    public:
        void Start() override;
        void Update() override;

        // 플레이어에서 에임포인터로의 방향 벡터 반환
        engine::Vector3 GetDirectionFrom(const engine::Vector3& fromPosition) const;
        
        // 현재 에임포인터의 월드 위치
        const engine::Vector3& GetWorldPosition() const { return m_worldPosition; }

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;
    };
}
