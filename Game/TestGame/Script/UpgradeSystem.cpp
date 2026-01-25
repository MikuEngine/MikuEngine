#include "GamePCH.h"
#include "UpgradeSystem.h"

namespace game
{
    void UpgradeSystem::Awake()
    {
        BuildDefaultTreeIfEmpty();
        RecomputeUnlocked();
    }

    void UpgradeSystem::Start()
    {

    }

    void UpgradeSystem::Update()
    {

    }

    void UpgradeSystem::OnGui()
    {

    }

    void UpgradeSystem::Save(engine::json& j) const
    {
        Object::Save(j);
        j["Currency"] = m_currency;
    }

    void UpgradeSystem::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "Currency", m_currency);

    }

    bool UpgradeSystem::CanUpgrade(int nodeId) const
    {
        return false;
    }

    bool UpgradeSystem::ApplyUpgrade(int nodeId)
    {
        return false;
    }

    void UpgradeSystem::BuildDefaultTreeIfEmpty()
    {

    }

    void UpgradeSystem::RecomputeUnlocked()
    {

    }
}