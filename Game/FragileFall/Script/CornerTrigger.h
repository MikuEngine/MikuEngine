#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // CornerTrigger - 맵 모서리 트리거
    // 
    // 용도:
    //   - 맵 모서리에 배치하여 동글 회색 몬스터의 회전 방향 결정에 사용
    //   - 막힌 방향 2개를 지정하여, 몬스터가 해당 방향으로 이동할 수 없음을 알림
    // 
    // 사용법:
    //   1. 맵 모서리에 트리거 콜라이더를 가진 오브젝트 배치
    //   2. 이 스크립트 추가
    //   3. OnGui에서 막힌 방향 2개 선택 (예: TopRight 모서리면 +X, +Z 막힘)
    // ═══════════════════════════════════════════════════════════════

    class CornerTrigger : public engine::Script<CornerTrigger>
    {
        REGISTER_SCRIPT(CornerTrigger, Script)

    public:
        // ─────────────────────────────────────────────
        // 막힌 방향 열거형 (월드 좌표 기준)
        // ─────────────────────────────────────────────
        enum class BlockedDirection : uint8_t
        {
            None = 0,
            PlusX,   // +X (우)
            MinusX,  // -X (좌)
            PlusZ,   // +Z (상)
            MinusZ   // -Z (하)
        };

    private:
        // ─────────────────────────────────────────────
        // 막힌 방향 2개 (모서리이므로 항상 2개)
        // ─────────────────────────────────────────────
        BlockedDirection m_blockedDir1 = BlockedDirection::PlusX;
        BlockedDirection m_blockedDir2 = BlockedDirection::PlusZ;

    public:
        // ─────────────────────────────────────────────
        // 막힌 방향 조회
        // ─────────────────────────────────────────────
        BlockedDirection GetBlockedDir1() const { return m_blockedDir1; }
        BlockedDirection GetBlockedDir2() const { return m_blockedDir2; }
        
        // 특정 방향이 막혔는지 확인
        bool IsDirectionBlocked(BlockedDirection dir) const
        {
            return (dir == m_blockedDir1) || (dir == m_blockedDir2);
        }

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
