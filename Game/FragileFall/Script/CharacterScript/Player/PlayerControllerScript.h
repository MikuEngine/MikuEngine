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
    class AimModeController;
    class BulletFactory;
    
    // 발사 이벤트 콜백 타입
    using FireCallback = std::function<void()>;

    // 피격 이벤트 콜백
    using DamageCallback = std::function<void()>;
    
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
        // ─────────────────────────────────────────────
        // 컴포넌트 참조
        // ─────────────────────────────────────────────
        engine::Rigidbody* m_rigidbody = nullptr;
        engine::AfterimageRenderer* m_afterimage = nullptr;

        AimModeController* m_aimPointer = nullptr;
        BulletFactory* m_bulletFactory = nullptr;


        // 이하 주석처리 된 멤버변수는 m_temperBase, m_temperFinal로 대체됨.
        // 기존 멤버 중 PCS단독으로 사용하는 변수만 살려둠

        // ═══════════════════════════════════════════════════════════════
        // 공격 변수 - Base값 (Save/Load 대상, OnGui 편집 가능)        //
        // ═══════════════════════════════════════════════════════════════
        //float m_baseAtkDmg = 10.0f;             // 기본 공격력
        //float m_baseAtkSpeed = 1.0f;            // 기본 공격속도 스케일 (1.0 = 초당 1.4발)
        //float m_baseBulletLifetime = 3.0f;      // 기본 총알 수명 (초)
        //float m_baseBulletRange = 50.0f;        // 기본 총알 사거리 (BulletPlayer 전용)
        //float m_baseBulletSizeScale = 0.7f;     // 기본 총알 스케일 (Prefab 기본 1.0 × 0.7 = 0.7)
        //float m_baseBulletSpeed = 1.0f;         // 기본 총알 속도

        // ═══════════════════════════════════════════════════════════════
        // 공격 변수 - 실제값 (PlayerTemperManager가 설정, OnGui 조회만)        // 
        // ═══════════════════════════════════════════════════════════════
        //float m_playerAtkDmg = 10.0f;           // 실제 공격력
        //float m_AtkSpeed = 1.0f;                // 실제 공격속도 스케일
        //float m_fireRate = 0.7f;                // 발사 간격 (초). m_AtkSpeed로부터 계산됨. 0.7 / m_AtkSpeed
        //float m_bulletLifetime = 3.0f;          // 실제 총알 수명 (초) - BulletPlayer는 사용 안 함
        //float m_bulletRange = 50.0f;            // 실제 총알 사거리 (BulletPlayer 전용)
        //float m_bulletSizeScale = 0.7f;         // 실제 총알 스케일
        //float m_bulletSpeed = 1.0f;             // 실제 총알 속도
        //bool m_isBulletDouble = false;          // 더블샷 (PlayerTemperManager가 설정)
        //float m_moveSpeed = 13.0f;
        //float m_dashImpulseMultiplier = 15.0f;          // 대쉬 Impulse 배율 (m_moveSpeed 기준)
        //float m_dashCooldown = 0.2f;                    // 대쉬 쿨다운 (초)




        //HP
        float m_PlayerMaxHP = 100.0f;
        float m_PlayerCurrentHP = 100.0f;

        // ─────────────────────────────────────────────
        // 프레자일 게이지 (몬스터가 있을 때만 상승, StageManager 연동)
        // ─────────────────────────────────────────────
        float m_fragileGaugeCurrent = 0.0f;
        float m_fragileGaugeMax = 100.0f;           // GUI·직렬화로 조정
        float m_fragileGaugeRisePerSecond = 15.0f;  // 몬스터 있을 때 초당 상승량

        // ─────────────────────────────────────────────
        // 이동 설정
        // ─────────────────────────────────────────────
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
        float m_dashDecayRate = 4.0f;                   // 대쉬 지수 감쇠율 (높을수록 빠르게 감속)
        int m_MaxDashCount = 3;
        int m_CurrentDashCount = 3;
        float m_dashRechargeTime = 1.0f;
        float m_dashRechargeTimer = 1.0f;
        /** 잔상 녹화 구간: 대쉬 시간의 이 비율까지만 RecordSample (0.7 = 전 구간의 70%만, 감속 구간 제외) */
        float m_dashAfterimageCutoffRatio = 0.7f;

        // ─────────────────────────────────────────────
        // 대쉬 런타임 상태
        // ─────────────────────────────────────────────
        bool m_isDashing = false;                       // 대쉬 중 여부 (FSM 동기화)
        float m_dashCooldownTimer = 0.0f;               // 쿨다운 타이머
        float m_dashElapsedTime = 0.0f;                 // 대쉬 경과 시간
        engine::Vector3 m_dashDirection = engine::Vector3::Zero;  // 대쉬 방향 (시작 시 고정)
        
        // ─────────────────────────────────────────────
        // 대쉬 감속 시스템 (타이머와 독립적)
        // - 타이머 종료 후에도 감속은 계속 진행
        // - 최종 이동속도까지만 감속 (버프 포함)
        // ─────────────────────────────────────────────
        bool m_isDashDecaying = false;                  // 감속 진행 중 여부
        float m_dashDecayStartSpeed = 0.0f;             // 감속 시작 시 속도
        float m_dashDecayElapsedTime = 0.0f;            // 감속 경과 시간
        float m_dashDecayDuration = 0.5f;               // 감속 지속 시간 (초)
             
        



        // 처형 관련
        float m_exeFragileRegen = 20.0f;
        float m_exeSplashDmg = 10.0f;
        float m_exeSplashRange = 5.0f;
        int   m_exeDashChargeRegen = 1;
        float m_exeHpRegen = 5.0f;

        // 체력/생존 관련
        float m_hpRegenOnClear = 10.0f;
        float m_fragileMax = 100.0f;
        float m_fragileRegenOnClear = 20.0f;
        float m_fragileGainRate = 1.0f; // 배율
        float m_invincibleTime = 0.5f;

        // 대시 관련
        float m_dashInvincibleTime = 0.1f;

        // ─────────────────────────────────────────────
        // 총알 발사 위치 (소켓 사용 시 오프셋 무시)
        // ─────────────────────────────────────────────
        /** BulletFireSocket이 있는 메쉬 오브젝트 이름. 씬에서 FindGameObject로 검색 */
        std::string m_playerAnimMeshObjectName = "PlayerAnimMesh";
        /** 발사 소켓 이름. 해당 메쉬의 SkeletalMeshRenderer에서 검색. 비우면 오프셋 방식 */
        std::string m_bulletFireSocketName = "BulletFireSocket";
        float m_bulletStartOffsetY = 2.2f;        // 소켓 미사용 시 Y축 발사 높이
        float m_bulletStartOffsetForward = 1.5f;  // 소켓 미사용 시 발사 방향 오프셋
        
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
        bool m_canFireNow = true;       // 애니 발사 프레임 통과 시 AimMeshController가 true로 설정
        bool m_hasFiredThisSession = false;  // 이번 클릭/홀드에서 1발 이상 쐈는지 (첫 발은 즉시 허용용)
        
        // 대쉬 후 즉시 발사 시스템
        float m_postDashQuickFireTimer = 0.0f;       // 대쉬 후 즉시 발사 가능 남은 시간
        float m_postDashQuickFireDuration = 0.3f;    // 대쉬 후 즉시 발사 가능 지속 시간 (1회만)

        float m_IdleDransitionWaitTime = 0.1f;
        float m_IdleDransitionWaitTimer = 0.0f;
        
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
       
    private:
        // PlayerTemperManager 전달

        // =========================
        // Temper Base (저장/편집 대상)
        // =========================
        struct TemperBaseStats
        {
            // Attack
            float atkDmg = 10.0f;
            float atkSpeed = 1.0f;          // 스케일(1.0 기준)
            float bulletRange = 50.0f;
            float bulletSizeScale = 0.7f;
            float bulletSpeed = 1.0f;

            // Execution
            float exeFragileRegen = 20.0f;
            float exeRange = 10.0f;
            float exeSplashDmg = 10.0f;
            float exeSplashRange = 5.0f;
            float exeDashChargeRegen = 1.0f; // apply에서 float로 받고 내부 int 변환 가능
            float exeHpRegen = 5.0f;

            // Health
            float hpRegenOnClear = 10.0f;
            float fragileMax = 100.0f;
            float fragileRegenOnClear = 20.0f;
            float fragileGainRate = 1.0f;
            float dashInvincibleTime = 0.1f;

            // Vitality / Movement
            float maxHp = 100.0f;
            float invincibleTime = 0.5f;
            float moveSpeed = 13.0f;

            // Dash
            float dashDistance = 15.0f;     // 현재는 dashImpulseMultiplier 역할로 쓰는 값
            float dashCooldown = 0.2f;
        };

        // =========================
        // Temper Final (런타임/표시용)
        // =========================
        struct TemperFinalStats
        {
            // Attack
            float atkDmg = 10.0f;
            float atkSpeed = 1.0f;
            float fireRate = 0.7f;          // 0.7 / atkSpeed
            float bulletRange = 50.0f;
            float bulletSizeScale = 0.7f;
            float bulletSpeed = 1.0f;

            // Execution
            float exeFragileRegen = 20.0f;
            float exeRange = 10.0f;
            float exeSplashDmg = 10.0f;
            float exeSplashRange = 5.0f;
            int   exeDashChargeRegen = 1;
            float exeHpRegen = 5.0f;

            // Health
            float hpRegenOnClear = 10.0f;
            float fragileMax = 100.0f;
            float fragileRegenOnClear = 20.0f;
            float fragileGainRate = 1.0f;
            float dashInvincibleTime = 0.1f;

            // Vitality / Movement
            float maxHp = 100.0f;
            float invincibleTime = 0.5f;
            float moveSpeed = 13.0f;

            // Dash
            float dashDistance = 15.0f;
            float dashCooldown = 0.2f;
        };

        TemperBaseStats  m_temperBase;
        TemperFinalStats m_temperFinal;

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
        void OnStateExited(const std::string& state) override;
        
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
        void StartDashDecay();           // 대쉬 감속 시작
        void HandleDashDecay();          // 대쉬 감속 처리 (FixedUpdate에서 호출)
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

        // 대미지 처리
        DamageCallback m_onDamaged = nullptr;   // 피격 콜백 저장용

    public:
        // ─────────────────────────────────────────────
        // 처형 시스템 접근자
        // ─────────────────────────────────────────────
        float GetExecutionRange() const { return m_executionRange; }

        // ─────────────────────────────────────────────
        // 대쉬 시스템 접근자
        // ─────────────────────────────────────────────
        bool IsDashing() const { return m_isDashing; }
        const engine::Vector3& GetDashDirection() const { return m_dashDirection; }
        int GetMaxDashCount() const { return m_MaxDashCount; }
        int GetCurrentDashCount() const { return m_CurrentDashCount; }
        float GetDashRechargeTimer() const { return m_dashRechargeTimer; }
        float GetDashRechargeTime() const { return m_dashRechargeTime; }

        // ─────────────────────────────────────────────
        // HP 시스템 접근자
        // ─────────────────────────────────────────────
        float GetMaxHp() const { return m_PlayerMaxHP; }
        float GetCurrentHp() const { return m_PlayerCurrentHP; }
        
        // 데미지 처리
        void SetOnDamaged(DamageCallback callback) { m_onDamaged = std::move(callback); }
        void TakeDamage(float damage);

        // 프레자일 게이지 (몬스터 있을 때 상승, UI/직렬화용)
        float GetFragileGaugeCurrent() const { return m_fragileGaugeCurrent; }
        float GetFragileGaugeMax() const { return m_fragileGaugeMax; }
        void SetFragileGaugeCurrent(float value) { m_fragileGaugeCurrent = value; }

        // ─────────────────────────────────────────────
        // 발사 시스템 접근자
        // ─────────────────────────────────────────────
        void RegisterFireCallback(engine::ScriptBase* owner, const FireCallback& callback);
        
        // ─────────────────────────────────────────────
        // 애니메이션 발사 프레임 동기화용
        // ─────────────────────────────────────────────
        /** 애니메이션 발사 프레임에 도달했는지 확인 (PlayerAimMeshController에서 호출) */
        bool CanFireNow() const { return m_canFireNow; }
        void SetCanFireNow(bool canFire) { m_canFireNow = canFire; }

    public:
        // Getter
        float GetBaseAtkDmg() const { return m_temperBase.atkDmg; }
        float GetBaseAtkSpeed() const { return  m_temperBase.atkSpeed; }
        float GetBaseBulletRange() const { return m_temperBase.bulletRange; }
        float GetBaseBulletSizeScale() const { return m_temperBase.bulletSizeScale; }
        float GetBaseBulletSpeed() const { return m_temperBase.bulletSpeed; }

        float GetBaseExeFragileRegen() const { return m_temperBase.exeFragileRegen; }
        float GetBaseExeRange() const { return m_temperBase.exeRange; }
        float GetBaseExeSplashDmg() const { return m_temperBase.exeSplashDmg; }
        float GetBaseExeSplashRange() const { return m_temperBase.exeSplashRange; }
        float GetBaseExeDashChargeRegen() const { return m_temperBase.exeDashChargeRegen; }
        float GetBaseExeHpRegen() const { return m_temperBase.exeHpRegen; }

        float GetBaseMaxHp() const { return m_temperBase.maxHp; }
        float GetBaseInvincibleTime() const { return m_temperBase.invincibleTime; }
        float GetBaseMoveSpeed() const { return m_temperBase.moveSpeed; }
        float GetBaseDashDistance() const { return m_temperBase.dashDistance; }
        float GetBaseDashCooldown() const { return m_temperBase.dashCooldown; }

        float GetBaseHpRegenOnClear() const { return m_temperBase.hpRegenOnClear; }
        float GetBaseFragileMax() const { return m_temperBase.fragileMax; }
        float GetBaseFragileRegenOnClear() const { return m_temperBase.fragileRegenOnClear; }
        float GetBaseFragileGainRate() const { return m_temperBase.fragileGainRate; }
        float GetBaseDashInvincibleTime() const { return m_temperBase.dashInvincibleTime; }

        // Setter
        void SetPlayerAtkDmg(float v) { m_temperFinal.atkDmg = v; }
        void SetAtkSpeed(float v) { m_temperFinal.atkSpeed = v; }
        void SetFireRate(float v) { m_temperFinal.fireRate = v; }

        void SetBulletRange(float v) { m_temperFinal.bulletRange = v; }
        void SetBulletSizeScale(float v) { m_temperFinal.bulletSizeScale = v; }
        void SetBulletSpeed(float v) { m_temperFinal.bulletSpeed = v; }

        void SetExeFragileRegen(float v) { m_temperFinal.exeFragileRegen = v; }
        void SetExeRange(float v) { m_temperFinal.exeRange = v; }
        void SetExeSplashDmg(float v) { m_temperFinal.exeSplashDmg = v; }
        void SetExeSplashRange(float v) { m_temperFinal.exeSplashRange = v; }
        void SetExeDashChargeRegen(float v) { m_temperFinal.exeDashChargeRegen = static_cast<int>(v); }
        void SetExeHpRegen(float v) { m_temperFinal.exeHpRegen = v; }

        void SetMaxHp(float v) { m_temperFinal.maxHp = v; }
        void SetCurrentHp(float v) { m_PlayerCurrentHP = v; }
        void SetInvincibleTime(float v) { m_temperFinal.invincibleTime = v; }
        void SetMoveSpeed(float v) { m_temperFinal.moveSpeed = v; }
        void SetDashDistance(float v) { m_temperFinal.dashDistance = v; }
        void SetDashCooldown(float v) { m_temperFinal.dashCooldown = v; }

        void SetHpRegenOnClear(float v) { m_temperFinal.hpRegenOnClear = v; }
        void SetFragileMax(float v) { m_temperFinal.fragileMax = v; }
		void SetCurrentFragile(float v) { m_fragileGaugeCurrent = v; }
        void SetFragileRegenOnClear(float v) { m_temperFinal.fragileRegenOnClear = v; }
        void SetFragileGainRate(float v) { m_temperFinal.fragileGainRate = v; }
        void SetDashInvincibleTime(float v) { m_temperFinal.dashInvincibleTime = v; }

        float GetAtkSpeed() const { return m_temperFinal.atkSpeed; }
        float GetFireRate() const { return m_temperFinal.fireRate; }

        void SetAfterImage(engine::AfterimageRenderer* comp) { m_afterimage = comp; }
        engine::AfterimageRenderer* GetAfterImage() const { return m_afterimage; }

        // 처형: Execution 상태일 때 재생 중인 처형 애니메이션의 정규화 시간 (0~1). 아니면 -1
        float GetExecutionAnimNormalizedTime() const;

        // 처형 시작 (ExecutionIndicatorManager에서 호출)
        void StartExecution(engine::GameObject* targetMonster);

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
