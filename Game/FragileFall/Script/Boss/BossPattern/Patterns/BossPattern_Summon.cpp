#include "GamePCH.h"
#include "BossPattern_Summon.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>

#include "Script/Boss/BossScript.h"

namespace game
{
    void BossPattern_Summon::Start(BossScript* boss)
    {
        if (!boss) return;

        m_isActive = true;
        m_intervalTimer = 0.0f;

        // 시작 시 즉시 소환
        SummonMonsters(boss);
    }

    void BossPattern_Summon::Update(BossScript* boss, float deltaTime)
    {
        if (!boss) return;

        // intervalTimer 업데이트
        m_intervalTimer += deltaTime;

        // interval이 지나면 소환
        if (m_intervalTimer >= m_interval)
        {
            SummonMonsters(boss);
            m_intervalTimer = 0.0f;
        }
    }

    void BossPattern_Summon::End(BossScript* boss)
    {
        m_isActive = false;
        m_intervalTimer = 0.0f;
    }

    int BossPattern_Summon::GetMonsterID(MonsterType type, BossScript::BossColor color) const
    {
        // 색상별 기본 오프셋
        int colorOffset = 0;
        switch (color)
        {
        case BossScript::BossColor::Red:    colorOffset = 0; break;
        case BossScript::BossColor::Blue:   colorOffset = 3; break;
        case BossScript::BossColor::Green:   colorOffset = 6; break;
        case BossScript::BossColor::Yellow: colorOffset = 9; break;
        case BossScript::BossColor::Purple: colorOffset = 12; break;
        }

        // 타입별 오프셋
        int typeOffset = 0;
        switch (type)
        {
        case MonsterType::Type1: typeOffset = 0; break;
        case MonsterType::Type2: typeOffset = 1; break;
        case MonsterType::Type3: typeOffset = 2; break;
        }

        // MonsterID = 색상 오프셋 + 타입 오프셋 + 1 (1부터 시작)
        // TODO: 실제 MonsterID 매핑으로 변경 필요
        return colorOffset + typeOffset + 1;
    }

    void BossPattern_Summon::SummonMonsters(BossScript* boss)
    {
        if (!boss || !boss->GetGameObject()) return;

        auto* scene = engine::SceneManager::Get().GetScene();
        if (!scene) return;

        auto* bossTransform = boss->GetGameObject()->GetTransform();
        if (!bossTransform) return;

        engine::Vector3 bossPos = bossTransform->GetWorldPosition();
        BossScript::BossColor currentColor = boss->GetCurrentColor();

        // 몬스터 소환
        for (int i = 0; i < m_summonCount; ++i)
        {
            // 랜덤 타입 선택
            MonsterType type = static_cast<MonsterType>(rand() % 3);

            // MonsterID 가져오기
            int monsterID = GetMonsterID(type, currentColor);

            // 소환 위치 계산 (보스 주변 원형 배치)
            float angle = static_cast<float>(i) * (360.0f / m_summonCount) * 3.14159f / 180.0f;
            float radius = m_summonRadius * (0.7f + (static_cast<float>(rand() % 100) / 100.0f) * 0.3f);  // 70%~100% 반경

            engine::Vector3 summonPos(
                bossPos.x + std::cosf(angle) * radius,
                bossPos.y,
                bossPos.z + std::sinf(angle) * radius
            );

            // 몬스터 GameObject 생성
            std::string monsterName = "BossSummonedMonster_" + std::to_string(monsterID) + "_" + std::to_string(i);
            auto* monsterGO = scene->CreateGameObject(monsterName);
            if (!monsterGO) continue;

            // Transform 설정
            auto* monsterTransform = monsterGO->GetTransform();
            if (monsterTransform)
            {
                monsterTransform->SetLocalPosition(summonPos);
            }

            // TODO: MonsterScript 컴포넌트 추가
            // TODO: MonsterID에 따라 적절한 몬스터 타입 스크립트 추가
            // 예: if (monsterID == 1) { monsterGO->AddComponent<MonsterPointedGreen>(); }
            //     else if (monsterID == 2) { monsterGO->AddComponent<MonsterDullGray>(); }
            //     ...

            // TODO: StaticMeshRenderer, Rigidbody, Collider 등 필요한 컴포넌트 추가
        }
    }
}
