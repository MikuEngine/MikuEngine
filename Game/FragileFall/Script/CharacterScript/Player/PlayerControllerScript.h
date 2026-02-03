#pragma once

#include "Script/CharacterScript/Common/BaseControllerScript.h"
#include "Script/CharacterScript/Common/BulletParams.h"
#include <Framework/Object/Ptr.h>
#include <functional>
#include <vector>

namespace engine
{
    class Rigidbody;
    class AfterimageRenderer;
    class ScriptBase;
}

namespace game
{
    class AimPointer;
    class BulletFactory;
    
    // 발사 이벤트 콜백 타입
    using FireCallback = std::function<void()>;
    
    // 발사 콜백 엔트리 (Ptr 기반 자동 유효성 관리)
    struct FireCallbackEntry
    {
        engine::Ptr<engine::ScriptBase> owner;  // 콜백 소유자 (파괴되면 자동 invalid)
        FireCallback callback;
    };

    // ═══════════════════════════════════════════════════════════════
    // PlayerControllerScript - Dynamic Rigidbody + 회전 제약 슈팅 플레이어
    // 
    // 아키텍처:
    //   - LogicFSM: 고수준 상태 (Idle, Walk, IdleShoot, WalkShoot)
    //   - 행동 로직: 함수 기반, CanMove()/CanAttack()으로 제한
    //   - AnimFSM: LogicFSM 상태에 따라 애니메이션 재생
    //   - Dynamic Rigidbody: PhysX 물리 충돌 처리, AddForce 기반 이동
    //   - 회전 제약: FreezeRotation으로 물리 회전 동결, 스크립트로 회전 제어
    // 
    // 물리 처리:
    //   - 이동: AddForce(VelocityChange)로 즉각적인 속도 변경
    //   - 충돌: PhysX가 자동 처리 (터널링 방지, 겹침 방지)
    //   - 회전: Transform.SetRotation()으로 직접 제어 (물리 무시)
    //   - 대쉬: AddForce(Impulse)로 순간 가속
    // 
    // FSM 상태:
    //   - Idle: 정지 상태
    //   - Walk: 이동 상태
    //   - IdleShoot: 정지 + 발사
    //   - WalkShoot: 이동 + 발사
    //   - Dash: 대쉬 상태
    //   - Execution: 처형 상태
    // ═══════════════════════════════════════════════════════════════
    class PlayerControllerScript : public BaseControllerScript
    {
        REGISTER_SCRIPT(PlayerControllerScript, BaseControllerScript)

    protected:
        engine::AfterimageRenderer* m_afterimage = nullptr;

        // ═══════════════════════════════════════════════════════════════
        // 공격 변수 - Base값 (Save/Load 대상, OnGui 편집 가능)
        // PlayerTemperManager가 이 값을 읽어서 강화 계산 후 실제값 설정
        // ═══════════════════════════════════════════════════════════════
        float m_baseAtkDmg = 10.0f;             // 기본 공격력
        float m_baseAtkSpeed = 1.0f;            // 기본 공격속도 스케일 (1.0 = 초당 1.4발)
        float m_baseBulletLifetime = 3.0f;      // 기본 총알 수명 (초)
        float m_baseBulletSizeScale = 1.0f;     // 기본 총알 스케일
        float m_baseBulletSpeed = 1.0f;         // 기본 총알 속도

        // ═══════════════════════════════════════════════════════════════
        // 공격 변수 - 실제값 (PlayerTemperManager가 설정, OnGui 조회만)
        // 공식: 실제값 = (Base + 합연산) × 곱연산
        // ═══════════════════════════════════════════════════════════════
        float m_playerAtkDmg = 10.0f;           // 실제 공격력
        float m_AtkSpeed = 1.0f;                // 실제 공격속도 스케일
        float m_fireRate = 0.7f;                // 발사 간격 (초). m_AtkSpeed로부터 계산됨. 0.7 / m_AtkSpeed
        float m_bulletLifetime = 3.0f;          // 실제 총알 수명 (초)
        float m_bulletSizeScale = 1.0f;         // 실제 총알 스케일
        float m_bulletSpeed = 1.0f;             // 실제 총알 속도
        bool m_isBulletDouble = false;          // 더블샷 (PlayerTemperManager가 설정)


        // ─────────────────────────────────────────────
        // 컴포넌트 참조
        // ─────────────────────────────────────────────
        engine::Rigidbody* m_rigidbody = nullptr;
        AimPointer* m_aimPointer = nullptr;
        BulletFactory* m_bulletFactory = nullptr;

        //HP
        int m_PlayerMaxHP = 10;
        int m_PlayerCurrentHP = 10;

        // ─────────────────────────────────────────────
        // 이동 설정
        // ─────────────────────────────────────────────
        float m_moveSpeed = 13.0f;
        float m_rotationSpeed = 10.0f;  // 회전 속도 (rad/sec)

        // ─────────────────────────────────────────────
        // 충돌 반응 설정
        // ─────────────────────────────────────────────
        float m_collisionPushBackForce = 0.5f;          // 정지 판정 시 밀어내기 힘 (Impulse)
        float m_slidingSpeedMultiplier = 0.7f;          // 슬라이딩 시 속도 배율 (0.0~1.0)
        
        // ─────────────────────────────────────────────
        // 대쉬 설정 (Dynamic Rigidbody + Impulse)
        // - PhysX가 충돌/이동 처리
        // - Impulse로 순간 가속 후 지수 감쇠
        // ─────────────────────────────────────────────
        float m_dashDuration = 0.3f;                    // 대쉬 지속 시간 (초)
        float m_dashImpulseMultiplier = 15.0f;          // 대쉬 Impulse 배율 (m_moveSpeed 기준)
        float m_dashDecayRate = 4.0f;                   // 대쉬 지수 감쇠율 (높을수록 빠르게 감속)
        float m_dashCooldown = 0.2f;                    // 대쉬 쿨다운 (초)
        int m_MaxDashCount = 3;
        int m_CurrentDashCount = 3;
        float m_dashRechargeTime = 1.0f;
        float m_dashRechargeTimer = 1.0f;

        // ─────────────────────────────────────────────
        // 대쉬 런타임 상태
        // ─────────────────────────────────────────────
        bool m_isDashing = false;                       // 대쉬 중 여부
        float m_dashCooldownTimer = 0.0f;               // 쿨다운 타이머
        float m_dashElapsedTime = 0.0f;                 // 대쉬 경과 시간
        engine::Vector3 m_dashDirection = engine::Vector3::Zero;  // 대쉬 방향 (시작 시 고정)
             
        
        // ─────────────────────────────────────────────
        // 총알 발사 위치 오프셋
        // ─────────────────────────────────────────────
        float m_bulletStartOffsetY = 2.2f;        // Y축 발사 높이
        float m_bulletStartOffsetForward = 1.5f;  // 발사 방향으로의 오프셋 (앞으로)
        
        // ─────────────────────────────────────────────
        // 발사 이벤트 콜백 (Ptr 기반 자동 정리)
        // ─────────────────────────────────────────────
        std::vector<FireCallbackEntry> m_fireCallbacks;

        // ─────────────────────────────────────────────
        // 처형 시스템 설정
        // ─────────────────────────────────────────────
        float m_executionRange = 10.0f;  // 처형 가능 거리

        // ─────────────────────────────────────────────
        // 참조 설정
        // ─────────────────────────────────────────────
        std::string m_aimPointerObjectName = "AimPointer";  // 씬에서 찾을 AimPointer 오브젝트 이름

        // ─────────────────────────────────────────────
        // 충돌 상태 추적 (PhysX 콜백 기반)
        // - 매 프레임 갱신 방식 (댕글링 방지)
        // - FixedUpdate 시작 시 클리어, OnCollisionStay에서 갱신
        // ─────────────────────────────────────────────
        bool m_isColliding = false;                      // 현재 충돌 중 여부
        std::vector<engine::Vector3> m_frameCollisionNormals;  // 현재 프레임 충돌 노말들 (여러 오브젝트 대응)
        
        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        float m_fireTimer = 0.0f;
        bool m_fsmInitialized = false;
        
        // 마지막 이동 방향 (캐릭터가 서있는 방향)
        // 초기값: -Z 방향 (아래쪽)
        engine::Vector3 m_lastMoveDirection = engine::Vector3(0.0f, 0.0f, -1.0f);

        // 캐릭터의 정면으로 사용할 벡터 == 현재 조준 방향
        // 오브젝트 트랜스폼과 관계 없음
        engine::Vector3 m_ForwardAimDir = engine::Vector3(0.0f, 0.0f, -1.0f);
        
        // 현재 캐릭터 하체 회전 각도 (라디안)
        float m_currentRotationAngle = 0.0f;
        
        // 정지 상태 목표 방향 (90도씩 양자화된 회전용)
        engine::Vector3 m_idleTargetDir = engine::Vector3(0.0f, 0.0f, 1.0f);
        bool m_idleTargetValid = false;  // 목표가 유효한지
        
        // 이동 상태 (Front/Back) - 히스테리시스용
        bool m_isWalkingBackward = false;
        
        // 이전 프레임 이동 상태 (이동 시작 감지용)
        bool m_wasMoving = false;       
       

    public:
        void Awake() override;
        void Start() override;

    protected:
        // ─────────────────────────────────────────────
        // BaseControllerScript 오버라이드
        // ─────────────────────────────────────────────
        void CacheComponents() override;
        void ProcessInput() override;
        void UpdateGameLogic() override;         // 비물리 로직 (타이머, 애니메이션 등)
        void UpdatePhysicsLogic() override;      // 물리 로직 (이동, 회전)
        void OnStateEntered(const std::string& state) override;
        
        // ─────────────────────────────────────────────
        // 충돌 콜백 (PhysX → CollisionSystem → Script)
        // ─────────────────────────────────────────────
        void OnCollisionEnter(const engine::CollisionInfo& info) override;
        void OnCollisionStay(const engine::CollisionInfo& info) override;
        void OnCollisionExit(const engine::CollisionInfo& info) override;

        // ─────────────────────────────────────────────
        // 행동 제한 (하이브리드 패턴)
        // ─────────────────────────────────────────────
        bool CanMove() const override;
        bool CanAttack() const override;

    private:
        // ─────────────────────────────────────────────
        // 초기화
        // ─────────────────────────────────────────────
        void InitializeFSM();

        // ─────────────────────────────────────────────
        // 입력 유틸리티
        // ─────────────────────────────────────────────
        engine::Vector3 GetMoveInputDirection() const;

        // ─────────────────────────────────────────────
        // 액션 함수
        // ─────────────────────────────────────────────
        
        void HandleDash();               // 대쉬 처리 (FixedUpdate에서 호출)
        void StartDash();                // 대쉬 시작
        void EndDash();                  // 대쉬 종료
        void HandleShooting(float deltaTime);  // Update에서 호출 (DeltaTime 사용)

        // ─────────────────────────────────────────────
        // Dynamic Rigidbody 설정
        // ─────────────────────────────────────────────
        void SetupDynamicRigidbody();   // Start()에서 호출, 회전 제약 설정
        void ForceStopRotation();       // FixedUpdate()에서 호출, 각속도 0 강제
        
        // ─────────────────────────────────────────────
        // 충돌 기반 이동 제한
        // ─────────────────────────────────────────────
        bool IsMovingIntoCollision(const engine::Vector3& moveDirection) const;
        engine::Vector3 RemoveCollisionComponent(const engine::Vector3& velocity) const;
        
        // ─────────────────────────────────────────────
        // 에임 추적, 회전, Forward-Back 스위칭 유틸리티
        // ─────────────────────────────────────────────
            
        engine::Vector3 m_inputMoveDir = engine::Vector3(0.0f, 0.0f, 0.0f);
        engine::Vector3 m_playerLogicalForward;
        engine::Vector3 m_targetRotateDirLowerbodyLogical;

        void CheckForwardBack(engine::Vector3& forward, engine::Vector3& aimDir);
        bool m_isBackward = false;        

        engine::Vector3 m_prevAimDirection = engine::Vector3(0.0f, 0.0f, 1.0f);  // 이전 에임 방향 (정규화)
        engine::Vector3 m_currentAimDir = engine::Vector3(0.0f, 0.0f, 1.0f);
        engine::Vector3 m_targetAimDirection = engine::Vector3(0.0f, 0.0f, 1.0f);     

        float m_aimCrossProduct = 0.0f;       // 현재 방향과 목표 방향의 외적 값 (양수: 반시계, 음수: 시계)
        bool m_aimTrackingInitialized = false;


        // ─────────────────────────────────────────────
        // 에디터 검증
        // ─────────────────────────────────────────────
        bool ValidateComponents() const;  // 컴포넌트 유효성 검사

    public:
        // ─────────────────────────────────────────────
        // 처형 시스템 접근자
        // ─────────────────────────────────────────────
        float GetExecutionRange() const { return m_executionRange; }

        // ─────────────────────────────────────────────
        // 대쉬 시스템 접근자
        // ─────────────────────────────────────────────
        int GetMaxDashCount() const { return m_MaxDashCount; }
        int GetCurrentDashCount() const { return m_CurrentDashCount; }
        float GetDashRechargeTimer() const { return m_dashRechargeTimer; }
        float GetDashRechargeTime() const { return m_dashRechargeTime; }

        // ─────────────────────────────────────────────
        // HP 시스템 접근자
        // ─────────────────────────────────────────────
        int GetMaxHp() const { return m_PlayerMaxHP; }
        int GetCurrentHp() const { return m_PlayerCurrentHP; }
        
        // 데미지 처리
        void TakeDamage(int damage);

        // ─────────────────────────────────────────────
        // 발사 시스템 접근자
        // ─────────────────────────────────────────────
        float GetFireRate() const { return m_fireRate; }
        void RegisterFireCallback(engine::ScriptBase* owner, const FireCallback& callback);

        // ─────────────────────────────────────────────
        // 공격 변수 - Base값 Getter (PlayerTemperManager용)
        // ─────────────────────────────────────────────
        float GetBaseAtkDmg() const { return m_baseAtkDmg; }
        float GetBaseAtkSpeed() const { return m_baseAtkSpeed; }
        float GetBaseBulletLifetime() const { return m_baseBulletLifetime; }
        float GetBaseBulletSizeScale() const { return m_baseBulletSizeScale; }
        float GetBaseBulletSpeed() const { return m_baseBulletSpeed; }

        // ─────────────────────────────────────────────
        // 공격 변수 - 실제값 Setter (PlayerTemperManager용)
        // ─────────────────────────────────────────────
        void SetPlayerAtkDmg(float value) { m_playerAtkDmg = value; }
        void SetAtkSpeed(float value) { m_AtkSpeed = value; }
        void SetFireRate(float value) { m_fireRate = value; }
        void SetBulletLifetime(float value) { m_bulletLifetime = value; }
        void SetBulletSizeScale(float value) { m_bulletSizeScale = value; }
        void SetBulletSpeed(float value) { m_bulletSpeed = value; }
        void SetIsBulletDouble(bool value) { m_isBulletDouble = value; }
        void SetAfterImage(engine::AfterimageRenderer* comp) { m_afterimage = comp; }

        // 처형 시작 (ExecutionIndicatorManager에서 호출)
        void StartExecution(engine::GameObject* targetMonster);

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
