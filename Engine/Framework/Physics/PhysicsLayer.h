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
                "Layer7", "Layer8", "Layer9",
                "Layer10", "Layer11", "Layer12", "Layer13", "Layer14", "Layer15",
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
            
            // Player ↔ Projectile: 충돌 안 함 (자기 총알에 안 맞음)
            SetCollision(Player, Projectile, false);
            
            // Projectile ↔ Projectile: 충돌 안 함
            SetCollision(Projectile, Projectile, false);

            SetCollision(Player, Environment, true);
            
            // Player ↔ Enemy: 충돌함
            // (기본값 All이므로 별도 설정 불필요)
            
            // Enemy ↔ Projectile: 충돌함
            // (기본값 All이므로 별도 설정 불필요)
            
            // Enemy ↔ Enemy: 충돌 안 함 (적끼리 밀치지 않음)
            SetCollision(Enemy, Enemy, false);
            
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
