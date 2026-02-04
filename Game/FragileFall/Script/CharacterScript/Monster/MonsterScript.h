#pragma once

#include "Script/CharacterScript/Common/BaseControllerScript.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/CharacterScript/Common/BulletParams.h"
#include "Script/MonsterGenerator/MonsterData.h"

namespace engine
{
    class Rigidbody;
    class SkeletalAnimator;
    class SkeletalMeshRenderer;
    class PathfindingAgent;
}

namespace game
{
    class BulletFactory;
    class PlayerControllerScript;
    class ExecutionIndicatorManager;  // friend 선언용 전방 선언

    // ═══════════════════════════════════════════════════════════════
    // MonsterScript - 몬스터 공통 기반 클래스
    // 
    // 아키텍처:
    //   - BaseControllerScript 상속 (LogicFSM + AnimFSM 활용)
    //   - 공통 로직: 플레이어 추적, 거리 계산, 회전, 발사
    //   - 몬스터마다 공격 범위, 체력, 속도 등 차별화
    // 
    // 공통 기능:
    //   - 플레이어 감지 및 추적
    //   - 플레이어 방향으로 회전 (Y축만)
    //   - 쿨다운 기반 자동 발사
    //   - 체력 관리 (Fragile → 부활/Dead 전환)
    // 
    // 공통 스탯 변수 (6개):
    //   - m_Hp: 체력
    //   - m_moveSpeed: 이동속도
    //   - m_attackDamage: 공격력
    //   - m_fragileTime: Fragile 지속시간
    //   - m_difficulty: 난이도 (MonsterPartyGenerator용)
    //   - m_detectionRange: 인식범위
    // ═══════════════════════════════════════════════════════════════
    class MonsterScript : public BaseControllerScript
    {
        REGISTER_SCRIPT(MonsterScript, BaseControllerScript)

        // ExecutionIndicatorManager가 TriggerDeath() 등에 접근할 수 있도록 허용
        friend class ExecutionIndicatorManager;

    protected:
        // ─────────────────────────────────────────────
        // 컴포넌트 참조
        // ─────────────────────────────────────────────
        engine::Rigidbody* m_rigidbody = nullptr;
        engine::SkeletalAnimator* m_skeletalAnimator = nullptr;
        BulletFactory* m_bulletFactory = nullptr;
        engine::PathfindingAgent* m_pathfindingAgent = nullptr;  // 경로 찾기 에이전트

        // ─────────────────────────────────────────────
        // 플레이어 추적
        // ─────────────────────────────────────────────
        PlayerControllerScript* m_targetPlayer = nullptr;
        std::string m_targetPlayerObjectName = "Player";

        // ─────────────────────────────────────────────
        // 몬스터 분류 정보
        // ─────────────────────────────────────────────
        AttackType m_attackType = AttackType::Dull;
        MonsterTier m_monsterTier = MonsterTier::Gray;

        // ─────────────────────────────────────────────
        // 몬스터 데이터 구조체 (MonsterPartyGenerator 연동용)
        // ─────────────────────────────────────────────
        MonsterData m_monsterData;

        // ─────────────────────────────────────────────
        // 공통 스탯 변수 (6개) - 에디터에서 수정 가능
        // ─────────────────────────────────────────────
        float m_Hp = 100.0f;                 // 체력
        float m_maxHp = 100.0f;              // 최대 체력 (부활 시 회복용)
        float m_moveSpeed = 0.0f;            // 이동속도
        float m_attackDamage = 10.0f;        // 공격력
        float m_fragileTime = 5.0f;          // Fragile 지속시간 (초)
        int m_difficulty = 1;                // 난이도 (MonsterPartyGenerator용)
        float m_detectionRange = 15.0f;      // 인식범위

        // ─────────────────────────────────────────────
        // 기타 스탯 (기존 유지)
        // ─────────────────────────────────────────────
        float m_AttackRange = 5.0f;          // 공격 사거리

        // ─────────────────────────────────────────────
        // 이동/회전/발사 설정
        // ─────────────────────────────────────────────
        float m_rotationSpeed = 2.0f;        // 회전 속도 (rad/sec)
        float m_fireRate = 3.0f;             // 발사 간격 (초)
        float m_bulletSpeed = 1.0f;          // 총알 속도
        float m_bulletLifetime = 3.0f;       // 총알 수명 (초)
        float m_spreadAngle = 0.2f;          // 3방향 발사 퍼짐 각도 (라디안, Blue용)

        // ─────────────────────────────────────────────
        // 나선형 탄환 전용 (Curve/Red 타입에서만 사용)
        // - 다른 BulletType(Linear, Parabolic 등)에서는 무시됨
        // ─────────────────────────────────────────────
        float m_curvedAngularSpeed = 2.0f;   // 회전 속도 (rad/s)
        float m_curvedRadiusGrowth = 3.0f;   // 반지름 증가율 (m/s)

        engine::Vector3 m_fireOffset{ 0.0f, 1.5f, 0.0f };  // 발사 위치 오프셋

        // ─────────────────────────────────────────────
        // 포물선 총알 설정 (Parabolic 타입에서만 사용)
        // - 에디터 설정: bulletSpeed, ownGravity
        // - 자동 계산: launchAngle (발사 시점에 거리 기반 계산)
        // - m_useHighArc: 높은/낮은 발사각 선택
        // - 제한: v² / g >= AttackRange (최대 사거리가 어택레인지 이상)
        // - 다른 타입에서는 이 값들을 무시함
        // ─────────────────────────────────────────────
        float m_parabolicSpeed = 15.0f;       // 포물선 총알 속력 (m/s)
        float m_ownGravity = 9.8f;            // 자체 중력 가속도 (m/s²)
        bool m_useHighArc = false;            // true: 높은 발사각 (곡선 화려), false: 낮은 발사각 (빠른 도달)

        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        float m_fireTimer = 0.0f;
        bool m_fsmInitialized = false;
        
        // ─────────────────────────────────────────────
        // Fragile 부활 시스템
        // ─────────────────────────────────────────────
        float m_fragileTimer = 0.0f;         // Fragile 상태 경과 시간
        bool m_fragileTimerStarted = false;  // Fragile 타이머 시작 여부
        
        // ─────────────────────────────────────────────
        // 피격 효과 (Hit Flash)
        // ─────────────────────────────────────────────
        engine::SkeletalMeshRenderer* m_skeletalMeshRenderer = nullptr;
        float m_hitFlashTimer = 0.0f;
        float m_hitFlashDuration = 0.2f;         // 피격 효과 지속 시간 (초)
        engine::Vector4 m_originalColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        engine::Vector4 m_hitFlashColor{ 1.0f, 1.0f, 1.0f, 1.0f };  // 흰색
        bool m_isHitFlashing = false;
        
        // ─────────────────────────────────────────────
        // 애니메이션 이름 (자식 클래스에서 설정)
        // ─────────────────────────────────────────────
        //std::string m_animName_Attack = "Attack";  // 공격 애니메이션 이름

        // ─────────────────────────────────────────────
        // 회전 완료 판정 (물리 기반 회전에서는 더 큰 임계값 필요)
        // ─────────────────────────────────────────────
        static constexpr float ROTATION_THRESHOLD = 10.0f * 3.14159f / 180.0f;  // 10도 (라디안)

    public:
        virtual void Awake() override;
        virtual void Start() override;

    protected:
        // ─────────────────────────────────────────────
        // BaseControllerScript 오버라이드
        // ─────────────────────────────────────────────
        void CacheComponents() override;
        void ProcessInput() override;
        void UpdateGameLogic() override;         // 비물리 로직 (타이머, 체력 등)
        void UpdatePhysicsLogic() override;      // 물리 로직 (이동, 회전)
        void OnStateEntered(const std::string& state) override;

        // ─────────────────────────────────────────────
        // 행동 제한 (하이브리드 패턴)
        // ─────────────────────────────────────────────
        bool CanMove() const override;
        bool CanAttack() const override;

        // ─────────────────────────────────────────────
        // 초기화 (자식에서 오버라이드 필수)
        // ─────────────────────────────────────────────
        virtual void InitializeFSM() {}          // 자식에서 구현
        virtual void InitializeAnimFSM() {}      // 자식에서 구현
        virtual void InitializeAnimations() {}   // 자식에서 구현
        virtual void InitializeBullet() {}       // 자식에서 구현 (총알 설정)

        // ─────────────────────────────────────────────
        // 플레이어 추적 (Player 특화 wrapper 함수)
        // ─────────────────────────────────────────────
        void FindPlayer();
        
        float GetDistanceToPlayer() const;
        engine::Vector3 CalculateDirectionToPlayer() const;
        bool IsPlayerInRange() const;       
        bool IsLookingAtPlayer() const;
        bool m_isPlayerInRange = false;
        bool IsRotatedTowardsPlayer() const { return IsLookingAtPlayer(); }

        // 이건 일단 다이나믹, 키네마틱 겸용으로 구현해야함.
        void RotateTowardsPlayer();  // FixedUpdate에서 호출  
        
        // ─────────────────────────────────────────────
        // 경로 찾기 (PathfindingAgent 활용)
        // ─────────────────────────────────────────────
        float GetPathDistanceToPlayer() const;
        
        void StopAllMovement();
        
        // ─────────────────────────────────────────────
        // 공격 (자손 클래스에서 오버라이드)
        // ─────────────────────────────────────────────
        virtual void Attack(float deltaTime);

        // ─────────────────────────────────────────────
        // 포물선 발사각 자동 계산 (Parabolic 타입용)
        // - 입력: 발사 위치, 착탄점
        // - 사용: m_parabolicSpeed, m_ownGravity, m_useHighArc
        // - 출력: launchAngle (라디안)
        // - m_useHighArc=true: 높은 발사각 (곡선 화려)
        // - m_useHighArc=false: 낮은 발사각 (빠른 도달)
        // ─────────────────────────────────────────────
        bool CalculateParabolicLaunchAngle(
            const engine::Vector3& startPos,   // 발사 위치 (오프셋 적용된 실제 위치)
            const engine::Vector3& targetPos,  // 착탄점 (플레이어 XZ, Y=0)
            float& outAngleRad                 // 출력: 발사각 (라디안)
        ) const;
        
        // ─────────────────────────────────────────────
        // 상태별 행동 - 비물리 (Update에서 호출, DeltaTime 기반)
        // ─────────────────────────────────────────────
        virtual void UpdateStateBasedBehavior(const std::string& state, float deltaTime);
        virtual void ExecuteEngageBehaviorNonPhysics(float deltaTime);  // 공격 타이머 등
        virtual void ExecuteIdleBehaviorNonPhysics();
        virtual void ExecuteFragileBehaviorNonPhysics();
        virtual void ExecuteDeadBehaviorNonPhysics();

        // ─────────────────────────────────────────────
        // 상태별 행동 - 물리 (FixedUpdate에서 호출)
        // ─────────────────────────────────────────────
        virtual void UpdatePhysicsStateBasedBehavior(const std::string& state);
        virtual void ExecuteEngageBehaviorPhysics();    // 이동, 회전
        virtual void ExecuteIdleBehaviorPhysics();
        virtual void ExecuteFragileBehaviorPhysics();
        virtual void ExecuteDeadBehaviorPhysics();
        
        // ─────────────────────────────────────────────
        // 이동 (PathfindingAgent 활용, 이동 몬스터용)
        // ─────────────────────────────────────────────
        virtual void MoveTowardsPlayer();  // FixedUpdate에서 호출
        
        // ─────────────────────────────────────────────
        // 리지드바디 타입별 통합 이동 인터페이스
        // - Dynamic: AddForce 기반 (ApplyMovementForce)
        // - Kinematic: SetLinearVelocity 기반
        // - Static: 무시
        // ─────────────────────────────────────────────
        void MoveInDirection(const engine::Vector3& direction, float speed);

        // ─────────────────────────────────────────────
        // 체력 관리
        // ─────────────────────────────────────────────
        void CheckHealth();
        void TriggerFragile();
        void TriggerDeath();  // Execution에서 호출
        virtual void OnFragile();
        virtual void OnDeath();
        
        // ─────────────────────────────────────────────
        // Fragile 부활 시스템
        // ─────────────────────────────────────────────
        void UpdateFragileTimer(float deltaTime);  // Fragile 타이머 업데이트
        void ReviveFromFragile();                  // Fragile 시간 초과 시 부활
        virtual void OnRevive();                   // 부활 시 콜백 (자식에서 오버라이드 가능)
        
        // ─────────────────────────────────────────────
        // MonsterData 동기화
        // ─────────────────────────────────────────────
        void SyncMonsterData();                    // MonsterScript 값으로 MonsterData 동기화
        
        // ─────────────────────────────────────────────
        // Dead 상태 후 파괴 타이머
        // ─────────────────────────────────────────────
        float m_deathTimer = 0.0f;
        float m_deathDelay = 2.0f;  // Dead 상태 후 파괴까지 대기 시간 (초)
        bool m_deathTimerStarted = false;
        
        // ─────────────────────────────────────────────
        // 피격 효과
        // ─────────────────────────────────────────────
        void StartHitFlash();
        void UpdateHitFlash(float deltaTime);
        void EndHitFlash();
        
    public:
        // ─────────────────────────────────────────────
        // 외부 데미지 처리 (총알 등에서 호출)
        // ─────────────────────────────────────────────
        virtual void TakeDamage(float damage);
        
        // ─────────────────────────────────────────────
        // 총알 타입 확인 (UI 분기용)
        // - 자식 클래스에서 오버라이드하여 Parabolic 여부 반환
        // ─────────────────────────────────────────────
        virtual bool IsParabolicBullet() const { return false; }
        
        // ─────────────────────────────────────────────
        // 접근자 (Getters)
        // ─────────────────────────────────────────────
        AttackType GetAttackType() const { return m_attackType; }
        MonsterTier GetMonsterTier() const { return m_monsterTier; }
        const MonsterData& GetMonsterData() const { return m_monsterData; }
        
        // 공통 스탯 접근자
        float GetHp() const { return m_Hp; }
        float GetMaxHp() const { return m_maxHp; }
        float GetMoveSpeed() const { return m_moveSpeed; }
        float GetAttackDamage() const { return m_attackDamage; }
        float GetFragileTime() const { return m_fragileTime; }
        int GetDifficulty() const { return m_difficulty; }
        float GetDetectionRange() const { return m_detectionRange; }
        float GetAttackRange() const { return m_AttackRange; }
        
        // ─────────────────────────────────────────────
        // 상태 플래그
        // ─────────────────────────────────────────────
        bool m_isFragile = false;
        bool m_isDead = false;

        // ─────────────────────────────────────────────
        // 에디터 검증
        // ─────────────────────────────────────────────
        virtual bool ValidateComponents() const;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
