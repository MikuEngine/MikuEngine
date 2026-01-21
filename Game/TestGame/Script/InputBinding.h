#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    // 키 입력 상태
    enum class InputState
    {
        None,       // 입력 없음
        Pressed,    // 이번 프레임에 눌림
        Held,       // 누르고 있음
        Released    // 이번 프레임에 뗌
    };

    // 키 바인딩 정보
    struct KeyBinding
    {
        std::string name;                                       // 바인딩 이름 (예: "MoveUp", "Attack")
        DirectX::Keyboard::Keys key = DirectX::Keyboard::Keys::None;  // 바인딩된 키
        InputState state = InputState::None;                    // 현재 입력 상태
    };

    // 키 이름 변환 유틸리티
    namespace KeyUtils
    {
        const char* KeyToString(DirectX::Keyboard::Keys key);
        DirectX::Keyboard::Keys StringToKey(const std::string& str);
        const std::vector<std::pair<DirectX::Keyboard::Keys, const char*>>& GetAllKeys();
    }

    class InputBinding :
        public engine::Script<InputBinding>
    {
        REGISTER_COMPONENT(InputBinding)

    private:
        std::vector<KeyBinding> m_bindings;
        std::unordered_map<std::string, size_t> m_nameToIndex;  // 빠른 검색용

    public:
        void Start() override;
        void Update() override;

        // 바인딩 관리
        void AddBinding(const std::string& name, DirectX::Keyboard::Keys key = DirectX::Keyboard::Keys::None);
        void RemoveBinding(const std::string& name);
        void SetKey(const std::string& name, DirectX::Keyboard::Keys key);
        void RenameBinding(const std::string& oldName, const std::string& newName);

        // 입력 상태 조회
        InputState GetInputState(const std::string& name) const;
        bool IsPressed(const std::string& name) const;
        bool IsHeld(const std::string& name) const;
        bool IsReleased(const std::string& name) const;

        // 현재 눌린 키들의 이름 목록 반환
        std::vector<std::string> GetPressedBindings() const;
        std::vector<std::string> GetHeldBindings() const;
        std::vector<std::string> GetReleasedBindings() const;

        // 전체 바인딩 조회
        const std::vector<KeyBinding>& GetAllBindings() const { return m_bindings; }

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;

    private:
        void RebuildNameIndex();
        void UpdateInputStates();
    };
}
