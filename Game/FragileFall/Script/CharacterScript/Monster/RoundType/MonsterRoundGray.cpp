#include "GamePCH.h"
#include "MonsterRoundGray.h"

#include "Script/CharacterScript/Common/BulletFactory.h"

#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Object/Component/Renderer/SkeletalMeshRenderer.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::Awake()
    {
        MonsterRoundType::Awake();
        
        // Gray 등급 고정
        m_monsterTier = MonsterTier::Gray;
    }

    void MonsterRoundGray::Start()
    {
        MonsterRoundType::Start();

        // Gray 등급 색상 설정 (회색)
        if (auto* meshRenderer = GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>())
        {
            meshRenderer->SetBaseColor(engine::Vector4(0.5f, 0.5f, 0.5f, 1.0f));
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 총알 초기화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::InitializeBullet()
    {
        // Gray: 기본 Linear 총알
        m_bulletParams.type = BulletType::Linear;
        m_bulletParams.speed = m_bulletSpeed;
        m_bulletParams.lifetime = m_bulletLifetime;
        m_bulletParams.damage = static_cast<int>(m_attackDamage);
    }

    // ═══════════════════════════════════════════════════════════════
    // 공격 (Gray 전용)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::Attack(float deltaTime)
    {
        // 공격 애니메이션 타이머 업데이트
        m_attackAnimationTimer += deltaTime;

        // 발사 가능 상태
        if (m_fireTimer <= 0.0f)
        {
            if (m_bulletFactory && m_targetPlayer && m_targetPlayer->GetGameObject())
            {
                // Gray: 단순 직선 발사
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
    void MonsterRoundGray::OnGui()
    {
        ImGui::Text("=== MonsterRoundGray ===");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Tier: Gray");
        
        // 부모 클래스 OnGui 호출
        MonsterRoundType::OnGui();
    }

    void MonsterRoundGray::Save(engine::json& j) const
    {
        MonsterRoundType::Save(j);
        // Gray 전용 데이터 저장 (필요시 추가)
    }

    void MonsterRoundGray::Load(const engine::json& j)
    {
        MonsterRoundType::Load(j);
        // Gray 전용 데이터 로드 (필요시 추가)
    }
}
