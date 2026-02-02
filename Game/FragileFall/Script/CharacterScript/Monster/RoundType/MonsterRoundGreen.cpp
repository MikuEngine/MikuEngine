#include "GamePCH.h"
#include "MonsterRoundGreen.h"

#include "Script/CharacterScript/Common/BulletFactory.h"

#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Object/Component/Renderer/SkeletalMeshRenderer.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::Awake()
    {
        MonsterRoundType::Awake();
        
        // Green 등급 고정
        m_monsterTier = MonsterTier::Green;
    }

    void MonsterRoundGreen::Start()
    {
        MonsterRoundType::Start();

        // Green 등급 색상 설정 (녹색)
        if (auto* meshRenderer = GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>())
        {
            meshRenderer->SetBaseColor(engine::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 총알 초기화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::InitializeBullet()
    {
        // Green: 기본 Linear 총알 (추후 특수 패턴으로 변경 가능)
        m_bulletParams.type = BulletType::Linear;
        m_bulletParams.speed = m_bulletSpeed;
        m_bulletParams.lifetime = m_bulletLifetime;
        m_bulletParams.damage = static_cast<int>(m_attackDamage);
    }

    // ═══════════════════════════════════════════════════════════════
    // 공격 (Green 전용)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::Attack(float deltaTime)
    {
        // 공격 애니메이션 타이머 업데이트
        m_attackAnimationTimer += deltaTime;

        // 발사 가능 상태
        if (m_fireTimer <= 0.0f)
        {
            if (m_bulletFactory && m_targetPlayer && m_targetPlayer->GetGameObject())
            {
                // Green: 단순 직선 발사 (추후 특수 패턴으로 변경)
                engine::Vector3 direction = CalculateDirectionToPlayer();
                engine::Vector3 firePosition = GetTransform()->GetWorldPosition();
                
                m_bulletFactory->LinearFireMonster(firePosition, direction, m_bulletParams);

                // 공격 애니메이션 재생
                if (m_skeletalAnimator && !m_animName_EngageAttack.empty())
                {
                    m_skeletalAnimator->Play(m_animName_EngageAttack, false, 0, 1.0f);
                }

                // 발사 쿨타임 리셋
                m_fireTimer = m_fireRate;
            }
        }

        // 공격 애니메이션 완료 체크
        if (m_attackAnimationTimer >= m_attackAnimationDuration)
        {
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("AttackComplete", true);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::OnGui()
    {
        ImGui::Text("=== MonsterRoundGreen ===");
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Tier: Green");
        
        // 부모 클래스 OnGui 호출
        MonsterRoundType::OnGui();
    }

    void MonsterRoundGreen::Save(engine::json& j) const
    {
        MonsterRoundType::Save(j);
        // Green 전용 데이터 저장 (필요시 추가)
    }

    void MonsterRoundGreen::Load(const engine::json& j)
    {
        MonsterRoundType::Load(j);
        // Green 전용 데이터 로드 (필요시 추가)
    }
}
