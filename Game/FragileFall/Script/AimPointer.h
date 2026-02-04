#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/LogicFSM.h>
#include <Framework/Object/Component/AnimFSM.h>

namespace engine
{
    class UIImage;
    class RectTransform;
    class Canvas;
}

namespace game
{
    class AimPointer :
        public engine::Script<AimPointer>
    {
        REGISTER_SCRIPT(AimPointer, Script)

    private:
        engine::Vector3 m_worldPosition;  // 마우스 월드 좌표

        // UI 커서 컴포넌트 (씬의 Canvas 오브젝트에서 관리)
        engine::GameObject* m_cursorObject = nullptr;
        engine::UIImage* m_cursorImage = nullptr;
        engine::RectTransform* m_cursorRect = nullptr;
        engine::Canvas* m_canvas = nullptr;

        // Canvas 설정
        std::string m_canvasObjectName = "AimPointerCanvas";  // 씬에서 찾을 Canvas 오브젝트 이름

        // 커서 설정
        std::string m_cursorTexturePrimary = "Resource/Texture/Earth.png";
        std::string m_cursorTextureAlternate = "Resource/Texture/Moon.png";
        bool m_useAlternateCursor = false;
        engine::Vector2 m_cursorSize{ 32.0f, 32.0f };
        engine::Vector2 m_cursorPivot{ 0.5f, 0.5f };

        // 월드 좌표 계산 설정
        float m_targetPlaneY = 1.5f;  // 레이캐스트 대상 평면의 Y 높이 (총알 발사 높이)
        /** 서서 쏠 때는 0, 걸을 때만 적용되는 에임 Y 보정 (양수=위, 음수=아래). 서서 쏠 때는 Target Plane Y로 조정 */
        float m_aimYOffsetWhenMoving = 0.0f;
        bool m_isMoving = false;  // 플레이어가 이동 중인지 (외부에서 SetMoving으로 설정)

    public:
        /** 이동 중일 때만 Y 보정 적용. 서서 쏠 때는 0, 걸을 때만 m_aimYOffsetWhenMoving 사용 */
        void SetMoving(bool moving) { m_isMoving = moving; }

        void Start() override;
        void Update() override;

        // 플레이어에서 에임포인터 방향을 얻는 함수
        engine::Vector3 GetDirectionFrom(const engine::Vector3& fromPosition) const;

        // 현재 에임포인터 월드 위치
        const engine::Vector3& GetWorldPosition() const { return m_worldPosition; }

        // 커서 이미지 교체 헬퍼
        // 사용 예:
        //   SetCursorTexture(false); // 기본 커서
        //   SetCursorTexture(true);  // 대체 커서
        void SetCursorTexture(bool useAlternate);

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void EnsureUICursor();
        void UpdateWorldPositionFromMouse(const engine::Vector2& mousePos);
    };
}


