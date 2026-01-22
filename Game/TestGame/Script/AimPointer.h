#pragma once

#include <Framework/Object/Component/Script.h>

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
        REGISTER_COMPONENT(AimPointer, Script)

    private:
        engine::Vector3 m_worldPosition;  // 마우스 월드 좌표

        // UI 커서 컴포넌트
        engine::UIImage* m_cursorImage = nullptr;
        engine::RectTransform* m_cursorRect = nullptr;
        engine::Canvas* m_canvas = nullptr;

        // 커서 설정
        std::string m_cursorTexturePrimary = "Resource/Texture/Earth.png";
        std::string m_cursorTextureAlternate = "Resource/Texture/Moon.png";
        bool m_useAlternateCursor = false;
        engine::Vector2 m_cursorSize{ 32.0f, 32.0f };
        engine::Vector2 m_cursorPivot{ 0.5f, 0.5f };

    public:
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


