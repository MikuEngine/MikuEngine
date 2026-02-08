#pragma once

#include <cstdint>
#include <string>
#include <array>

namespace engine
{
    // ═══════════════════════════════════════════════════════════════
    // 물리 레이어 정의
    // 최대 32개 레이어 지원 (uint32_t 비트마스크)
    // ═══════════════════════════════════════════════════════════════

    namespace PhysicsLayer
    {
        // 레이어 인덱스 (0~31)
        enum Index : uint32_t
        {
            Default = 0,
            Player = 1,
            Enemy = 2,
            Projectile = 3,
            Environment = 4,
            Trigger = 5,
            EnemyProjectile = 6,
            Picking = 7,
            Field = 8,                      // 장판 공격
            Wall = 9,                       // 벽 (Projectile, EnemyProjectile 제외 모든 물체와 충돌)
            EnemyParabolicProjectile = 10,  // 포물선 적 투사체 (Wall, Player만 충돌)
            ExplosionTrigger = 11,          // 폭발 트리거 (Player, Wall만 충돌)
            JumpingEnemy = 12,              // 점프 중인 적 (Environment 충돌 무시, Wall/Player는 충돌)
            RadiusChecker = 13,             // 반경 체크용 디버그 오브젝트 (모든 충돌 무시)
            SplittingEnemy = 14,            // 분열 몬스터 (Enemy와 동일, 자기 자신끼리는 충돌 안 함)
            BossBulletProjectile = 15,      // 보스 총알 투사체 (Player, Environment, Wall만 충돌)
            // 확장 시 여기에 추가 (최대 31까지)
            
            Count = 32  // 최대 레이어 수
        };

        // 레이어 마스크 (비트 플래그)
        enum Mask : uint32_t
        {
            None = 0,
            DefaultMask = (1u << Default),
            PlayerMask = (1u << Player),
            EnemyMask = (1u << Enemy),
            ProjectileMask = (1u << Projectile),
            EnvironmentMask = (1u << Environment),
            TriggerMask = (1u << Trigger),
            EnemyProjectileMask = (1u << EnemyProjectile),
            PickingMask = (1u << Picking),
            FieldMask = (1u << Field),
            WallMask = (1u << Wall),
            EnemyParabolicProjectileMask = (1u << EnemyParabolicProjectile),
            ExplosionTriggerMask = (1u << ExplosionTrigger),
            JumpingEnemyMask = (1u << JumpingEnemy),
            RadiusCheckerMask = (1u << RadiusChecker),
            SplittingEnemyMask = (1u << SplittingEnemy),
            BossBulletProjectileMask = (1u << BossBulletProjectile),

            All = 0xFFFFFFFF
        };

        // 레이어 인덱스 → 마스크 변환
        constexpr uint32_t ToMask(uint32_t layerIndex)
        {
            return (1u << layerIndex);
        }

        // 레이어 이름 (디버그/에디터용)
        inline const char* GetLayerName(uint32_t layerIndex)
        {
            static const char* names[Count] = {
                "Default",      // 0
                "Player",       // 1
                "Enemy",        // 2
                "Projectile",   // 3
                "Environment",  // 4
                "Trigger",      // 5
                "EnemyProjectile", // 6
                "Picking",      // 7
                "Field",        // 8
                "Wall",         // 9
                "EnemyParabolicProjectile", // 10
                "ExplosionTrigger", // 11
                "JumpingEnemy", // 12
                "RadiusChecker", // 13
                "SplittingEnemy", // 14
                "BossBulletProjectile", // 15
                
                "Layer16", "Layer17", "Layer18", "Layer19",
                "Layer20", "Layer21", "Layer22", "Layer23", "Layer24", "Layer25",
                "Layer26", "Layer27", "Layer28", "Layer29", "Layer30", "Layer31"
            };
            
            if (layerIndex < Count)
            {
                return names[layerIndex];
            }
            return "Invalid";
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 레이어 충돌 매트릭스
    // 어떤 레이어끼리 충돌할지 정의
    // ═══════════════════════════════════════════════════════════════

    class PhysicsLayerMatrix
    {
    private:
        // matrix[A] = B와 충돌하는 레이어들의 마스크
        std::array<uint32_t, PhysicsLayer::Count> m_matrix;

    public:
        PhysicsLayerMatrix()
        {
            // 기본값: 모든 레이어가 서로 충돌
            m_matrix.fill(PhysicsLayer::Mask::All);
        }

        // 기본 설정으로 초기화
        void SetupDefault()
        {
            using namespace PhysicsLayer;
            
            // 모든 레이어 초기화 (기본: 모두 충돌)
            m_matrix.fill(Mask::All);
            
            // ═══════════════════════════════════════
            // 탑다운 슈팅 게임용 충돌 규칙
            // ═══════════════════════════════════════
            
            SetCollision(Player, Player, false);

            // Player ↔ Projectile: 충돌 안 함 (자기 총알에 안 맞음)
            SetCollision(Player, Projectile, false);
            
            // Projectile ↔ Projectile: 충돌 안 함
            SetCollision(Projectile, Projectile, false); 

            // Player ↔ Enemy: 충돌함 (PhysX가 충돌 처리)
            // 플레이어 Dynamic + 몬스터 Kinematic = 플레이어가 몬스터에게 막힘
            SetCollision(Player, Enemy, true);
            
            // Enemy ↔ Projectile: 충돌함
            // (기본값 All이므로 별도 설정 불필요)
            
            // Enemy ↔ Enemy: 충돌  
            
            // ═══════════════════════════════════════
            // EnemyProjectile 충돌 규칙
            // ═══════════════════════════════════════
            
            // EnemyProjectile ↔ Enemy: 충돌 안 함 (적 총알은 적에게 안 맞음)
            SetCollision(EnemyProjectile, Enemy, false);
            
            // EnemyProjectile ↔ EnemyProjectile: 충돌 안 함
            SetCollision(EnemyProjectile, EnemyProjectile, false);
            
            // EnemyProjectile ↔ Projectile: 충돌 안 함 (총알끼리 충돌 안 함)
            SetCollision(EnemyProjectile, Projectile, false);
            
            // EnemyProjectile ↔ Trigger: 충돌 안 함
            SetCollision(EnemyProjectile, Trigger, false);
            
            // EnemyProjectile ↔ Player: 충돌함 (기본값)
            // EnemyProjectile ↔ Default: 충돌함 (기본값)
            // EnemyProjectile ↔ Environment: 충돌함 (기본값)


            // ═══════════════════════════════════════
			// Feild (장판) 충돌 규칙
            // ═══════════════════════════════════════

            // Field ↔ Enemy: 충돌 안 함 (적 장판이 적에게 피해를 주지 않음)
            SetCollision(Field, Enemy, false);

            // Field ↔ EnemyProjectile: 충돌 안 함 (장판이 날아오는 총알을 막으면 안 됨)
            SetCollision(Field, EnemyProjectile, false);

            // Field ↔ Projectile: 충돌 안 함 (장판이 플레이어 총알을 막으면 안 됨)
            SetCollision(Field, Projectile, false);

            // Field ↔ Field: 충돌 안 함 (장판끼리 밀어내지 않음)
            SetCollision(Field, Field, false);

            // Field ↔ Player: 충돌함 (기본값 All에 의해 유지, Trigger로 작동 유도)
            // Field ↔ Environment: 충돌함 (바닥이나 벽 감지를 위해 유지)


            // ═══════════════════════════════════════
            // Picking 충돌 규칙
            // ═══════════════════════════════════════
            SetCollision(Picking, Default, false);
            SetCollision(Picking, Player, false);
            SetCollision(Picking, Enemy, false);
            SetCollision(Picking, Projectile, false);
            SetCollision(Picking, Environment, false);
            SetCollision(Picking, Trigger, false);            
            SetCollision(Picking, EnemyProjectile, false);
            
            // ═══════════════════════════════════════
            // Wall 충돌 규칙
            // Projectile, EnemyProjectile 제외 모든 물체와 충돌
            // ═══════════════════════════════════════
            SetCollision(Wall, Projectile, false);
            SetCollision(Wall, EnemyProjectile, false);
            // 나머지는 기본값 All에 의해 충돌함
            
            // ═══════════════════════════════════════
            // EnemyParabolicProjectile 충돌 규칙
            // Wall, Player만 충돌
            // ═══════════════════════════════════════
            SetCollision(EnemyParabolicProjectile, Default, false);
            SetCollision(EnemyParabolicProjectile, Enemy, false);
            SetCollision(EnemyParabolicProjectile, Projectile, false);
            SetCollision(EnemyParabolicProjectile, Environment, false);
            SetCollision(EnemyParabolicProjectile, Trigger, false);
            SetCollision(EnemyParabolicProjectile, EnemyProjectile, false);
            SetCollision(EnemyParabolicProjectile, Picking, false);
            SetCollision(EnemyParabolicProjectile, Field, false);
            SetCollision(EnemyParabolicProjectile, EnemyParabolicProjectile, false);
            // Wall, Player만 충돌 (기본값 All에서 위 항목들 제외)
            
            // ═══════════════════════════════════════
            // ExplosionTrigger 충돌 규칙
            // Player, Wall만 충돌
            // ═══════════════════════════════════════
            SetCollision(ExplosionTrigger, Default, false);
            SetCollision(ExplosionTrigger, Enemy, false);
            SetCollision(ExplosionTrigger, Projectile, false);
            SetCollision(ExplosionTrigger, Environment, false);
            SetCollision(ExplosionTrigger, Trigger, false);
            SetCollision(ExplosionTrigger, EnemyProjectile, false);
            SetCollision(ExplosionTrigger, Picking, false);
            SetCollision(ExplosionTrigger, Field, false);
            SetCollision(ExplosionTrigger, EnemyParabolicProjectile, false);
            SetCollision(ExplosionTrigger, ExplosionTrigger, false);
            // Player, Wall만 충돌 (기본값 All에서 위 항목들 제외)
            
            // ═══════════════════════════════════════
            // JumpingEnemy 충돌 규칙
            // Environment 충돌 무시, Wall/Player/Projectile은 충돌
            // ═══════════════════════════════════════
            SetCollision(JumpingEnemy, Environment, false);  // 점프 중 Environment 무시
            SetCollision(JumpingEnemy, Trigger, false);      // 트리거 무시
            // Projectile(플레이어 총알)은 기본값 All에 의해 충돌함
            // Wall, Player, Enemy 등은 기본값 All에 의해 충돌함
            
            // ═══════════════════════════════════════
            // RadiusChecker 충돌 규칙
            // 모든 레이어와 충돌하지 않음 (디버그 시각화 전용)
            // ═══════════════════════════════════════
            SetCollision(RadiusChecker, Default, false);
            SetCollision(RadiusChecker, Player, false);
            SetCollision(RadiusChecker, Enemy, false);
            SetCollision(RadiusChecker, Projectile, false);
            SetCollision(RadiusChecker, Environment, false);
            SetCollision(RadiusChecker, Trigger, false);
            SetCollision(RadiusChecker, EnemyProjectile, false);
            SetCollision(RadiusChecker, Picking, false);
            SetCollision(RadiusChecker, Field, false);
            SetCollision(RadiusChecker, Wall, false);
            SetCollision(RadiusChecker, EnemyParabolicProjectile, false);
            SetCollision(RadiusChecker, ExplosionTrigger, false);
            SetCollision(RadiusChecker, JumpingEnemy, false);
            SetCollision(RadiusChecker, SplittingEnemy, false);
            
            // ═══════════════════════════════════════
            // SplittingEnemy 충돌 규칙
            // Enemy와 동일하지만, 자기 자신(SplittingEnemy)끼리는 충돌 안 함
            // ═══════════════════════════════════════
            SetCollision(SplittingEnemy, SplittingEnemy, false);  // 분열 몬스터끼리 충돌 안 함
            // 나머지는 기본값 All에 의해 Enemy와 동일하게 충돌함
            SetCollision(RadiusChecker, RadiusChecker, false);
            
            // ═══════════════════════════════════════
            // BossBulletProjectile 충돌 규칙
            // Player만 충돌 (트리거)
            // ═══════════════════════════════════════
            SetCollision(BossBulletProjectile, Default, false);
            SetCollision(BossBulletProjectile, Enemy, false);
            SetCollision(BossBulletProjectile, Projectile, false);
            SetCollision(BossBulletProjectile, Environment, false);
            SetCollision(BossBulletProjectile, Trigger, false);
            SetCollision(BossBulletProjectile, EnemyProjectile, false);
            SetCollision(BossBulletProjectile, Picking, false);
            SetCollision(BossBulletProjectile, Field, false);
            SetCollision(BossBulletProjectile, Wall, false);
            SetCollision(BossBulletProjectile, EnemyParabolicProjectile, false);
            SetCollision(BossBulletProjectile, ExplosionTrigger, false);
            SetCollision(BossBulletProjectile, JumpingEnemy, false);
            SetCollision(BossBulletProjectile, RadiusChecker, false);
            SetCollision(BossBulletProjectile, SplittingEnemy, false);
            SetCollision(BossBulletProjectile, BossBulletProjectile, false);
            // Player만 충돌 (기본값 All에서 위 항목들 제외)
  
        }

        // 두 레이어 간 충돌 여부 설정
        void SetCollision(uint32_t layerA, uint32_t layerB, bool shouldCollide)
        {
            if (layerA >= PhysicsLayer::Count || layerB >= PhysicsLayer::Count)
                return;

            if (shouldCollide)
            {
                m_matrix[layerA] |= (1u << layerB);
                m_matrix[layerB] |= (1u << layerA);
            }
            else
            {
                m_matrix[layerA] &= ~(1u << layerB);
                m_matrix[layerB] &= ~(1u << layerA);
            }
        }

        // 특정 레이어가 충돌하는 레이어 마스크 얻기
        uint32_t GetCollisionMask(uint32_t layer) const
        {
            if (layer >= PhysicsLayer::Count)
                return 0;
            return m_matrix[layer];
        }

        // 두 레이어가 충돌하는지 확인
        bool ShouldCollide(uint32_t layerA, uint32_t layerB) const
        {
            if (layerA >= PhysicsLayer::Count || layerB >= PhysicsLayer::Count)
                return false;
            return (m_matrix[layerA] & (1u << layerB)) != 0;
        }

        // 전체 매트릭스 접근 (PhysX 필터 셰이더용)
        const std::array<uint32_t, PhysicsLayer::Count>& GetMatrix() const
        {
            return m_matrix;
        }
    };
}
