#pragma once

#include "CharacterLogicFSM.h"

namespace game
{
    class AimPointer;
    class TempBulletFactory;
    class CharacterAnimationFSM;

    // ═══════════════════════════════════════════════════════════════
    // PlayerFSM - CharacterLogicFSM을 상속받은 플레이어 전용 FSM
    // 
    // 추가 기능: 
    //   - WASD 이동, 마우스 클릭으로 총알 발사
    //   - AimPointer를 향해 발사
    //   - 에임 방향으로 상체 회전
    // ═══════════════════════════════════════════════════════════════
    class PlayerFSM :
        public CharacterLogicFSM
    {
        REGISTER_COMPONENT(PlayerFSM, Script)

    private:
        // 컴포넌트 참조
        AimPointer* m_aimPointer = nullptr;
        TempBulletFactory* m_bulletFactory = nullptr;
        CharacterAnimationFSM* m_charAnimFSM = nullptr;
        
        // 상체 회전 설정
        bool m_enableUpperBodyAim = true;

    public:
        void Awake() override;
        void Start() override;

    protected:
        void CacheComponents() override;
        
        // 입력 처리 (모든 입력을 여기서 처리)
        void ProcessInput() override;
        
        // 상태 Enter/Update 오버라이드
        void OnEnterState(CharacterState state) override;
        void UpdateCurrentState() override;

    private:
        // 플레이어 전용 입력 함수 (private)
        engine::Vector3 GetMoveInputDirection() const;
        
        // 플레이어 전용 액션
        void HandleShooting();
        void UpdateUpperBodyAim();
        float CalculateAimYaw() const;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
