#pragma once

#include "CharacterLogicFSM.h"

namespace game
{
    class AimPointer;
    class TempBulletFactory;

    // ═══════════════════════════════════════════════════════════════
    // PlayerFSM - CharacterLogicFSM을 상속받은 플레이어 전용 FSM
    // 
    // 추가 기능: 마우스 클릭으로 총알 발사
    // ═══════════════════════════════════════════════════════════════

    class PlayerFSM :
        public CharacterLogicFSM
    {
        REGISTER_COMPONENT(PlayerFSM)

    private:
        // 발사 관련
        AimPointer* m_aimPointer = nullptr;
        TempBulletFactory* m_bulletFactory = nullptr;

    public:
        void Start() override;
        void Update() override;

    protected:
        void CacheComponents() override;
        
        // Attack 상태를 마우스 클릭으로 오버라이드
        bool IsAttackPressed() const override;
        
        // 공격 시 총알 발사
        void OnEnterAttack() override;
        void UpdateAttack() override;

    private:
        void HandleShooting();

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;
    };
}
