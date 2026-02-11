#include "GamePCH.h"
#include "ExecutionExitDamageTrigger.h"

#include "Script/CharacterScript/Monster/MonsterScript.h"
#include "Script/Boss/BossScript.h"
#include "Script/Boss/BossPattern/Components/BossBigProjectile.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Object/Component/SphereCollider.h>
#include <Framework/Physics/PhysicsLayer.h>

namespace game
{
	void ExecutionExitDamageTrigger::Setup(float damage, float radiusScale, float lifetime)
	{
		m_damage = damage;
		m_radiusScale = radiusScale;
		m_lifetime = lifetime;
	}


	void ExecutionExitDamageTrigger::Start()
	{
		m_elapsedTime = 0.0f;
		m_damagedObjects.clear();

		// ═══════════════════════════════════════════════════════════════
		// 스케일 먼저 설정 (radiusScale에 맞춰 즉시 적용)
		// ═══════════════════════════════════════════════════════════════
		GetTransform()->SetLocalScale(engine::Vector3(m_radiusScale, m_radiusScale, m_radiusScale));

		// ═══════════════════════════════════════════════════════════════
		// SphereCollider 설정
		// - SyncWithTransform 활성화 → Transform 스케일로 자동 크기 조정
		// - SetRadius는 호출하지 않음 (프리팹 기본값 + 스케일로 자동 조정)
		// ═══════════════════════════════════════════════════════════════
		m_sphereCollider = GetGameObject()->GetComponent<engine::SphereCollider>();
		if (m_sphereCollider)
		{
			m_sphereCollider->SetLayer(engine::PhysicsLayer::EE_DamageTrigger);
			m_sphereCollider->SetIsTrigger(true);
			m_sphereCollider->SetSyncWithTransform(true);  // Transform 스케일 동기화 활성화
		}
	}

	void ExecutionExitDamageTrigger::Update()
	{
		m_elapsedTime += engine::Time::DeltaTime();

		// 생존 시간 초과 시 파괴
		if (m_elapsedTime >= m_lifetime)
		{
			GetGameObject()->Destroy();
		}
	}

	void ExecutionExitDamageTrigger::OnTriggerEnter(const engine::CollisionInfo& info)
	{
		if (!info.gameObject) return;

		LOG_PRINT("[EEDT] OnTriggerEnter: %s", info.gameObject->GetName().c_str());

		// raw pointer를 void*로 변환하여 중복 체크
		void* objPtr = info.gameObject.Get();

		// 이미 데미지를 받은 오브젝트인지 확인 (중복 방지)
		if (m_damagedObjects.count(objPtr) > 0)
		{
			LOG_PRINT("[EEDT] Already damaged: %s", info.gameObject->GetName().c_str());
			return;
		}

		// 각 타입별로 체크하여 데미지 적용
		// 대상: MonsterScript, BossScript, BossBigProjectile

		bool damaged = false;

		// MonsterScript (Enemy, JumpingEnemy, SplittingEnemy 등)
		if (auto* monster = info.gameObject->GetComponent<MonsterScript>())
		{
			LOG_PRINT("[EEDT] Damaging MonsterScript: %s (%.1f damage)", info.gameObject->GetName().c_str(), m_damage);
			monster->TakeDamage(m_damage);
			damaged = true;
		}
		// BossScript
		else if (auto* boss = info.gameObject->GetComponent<BossScript>())
		{
			LOG_PRINT("[EEDT] Damaging BossScript: %s (%.1f damage)", info.gameObject->GetName().c_str(), m_damage);
			boss->TakeDamage(m_damage);
			damaged = true;
		}
		// BossBigProjectile
		else if (auto* bigProj = info.gameObject->GetComponent<BossBigProjectile>())
		{
			LOG_PRINT("[EEDT] Damaging BossBigProjectile: %s (%.1f damage)", info.gameObject->GetName().c_str(), m_damage);
			bigProj->TakeDamage(m_damage);
			damaged = true;
		}
		else
		{
			LOG_PRINT("[EEDT] No damageable component found on: %s", info.gameObject->GetName().c_str());
		}

		// 데미지를 적용했으면 기록
		if (damaged)
		{
			m_damagedObjects.insert(objPtr);
			LOG_PRINT("[EEDT] Damage applied successfully. Total damaged: %zu", m_damagedObjects.size());
		}
	}
}
