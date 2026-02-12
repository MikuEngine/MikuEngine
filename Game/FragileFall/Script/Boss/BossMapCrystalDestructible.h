#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class BossScript;

    // 맵 수정 장식물: 시작 시 보스에 자신을 등록하여 클리어 연출에서 순차 파괴되게 한다.
    class BossMapCrystalDestructible :
        public engine::Script<BossMapCrystalDestructible>
    {
        REGISTER_SCRIPT(BossMapCrystalDestructible, Script)

    private:
        engine::Ptr<BossScript> m_registeredBoss = nullptr;

    public:
        void Start() override;
        void OnDestroy() override;
    };
}
