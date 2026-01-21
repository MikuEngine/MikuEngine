#include "GamePCH.h"
#include "InputBinding.h"

namespace game
{
    namespace KeyUtils
    {
        // 주요 키 목록 (필요에 따라 추가)
        static const std::vector<std::pair<DirectX::Keyboard::Keys, const char*>> s_keyList = {
            // 없음
            { DirectX::Keyboard::Keys::None, "None" },
            
            // 알파벳
            { DirectX::Keyboard::Keys::A, "A" },
            { DirectX::Keyboard::Keys::B, "B" },
            { DirectX::Keyboard::Keys::C, "C" },
            { DirectX::Keyboard::Keys::D, "D" },
            { DirectX::Keyboard::Keys::E, "E" },
            { DirectX::Keyboard::Keys::F, "F" },
            { DirectX::Keyboard::Keys::G, "G" },
            { DirectX::Keyboard::Keys::H, "H" },
            { DirectX::Keyboard::Keys::I, "I" },
            { DirectX::Keyboard::Keys::J, "J" },
            { DirectX::Keyboard::Keys::K, "K" },
            { DirectX::Keyboard::Keys::L, "L" },
            { DirectX::Keyboard::Keys::M, "M" },
            { DirectX::Keyboard::Keys::N, "N" },
            { DirectX::Keyboard::Keys::O, "O" },
            { DirectX::Keyboard::Keys::P, "P" },
            { DirectX::Keyboard::Keys::Q, "Q" },
            { DirectX::Keyboard::Keys::R, "R" },
            { DirectX::Keyboard::Keys::S, "S" },
            { DirectX::Keyboard::Keys::T, "T" },
            { DirectX::Keyboard::Keys::U, "U" },
            { DirectX::Keyboard::Keys::V, "V" },
            { DirectX::Keyboard::Keys::W, "W" },
            { DirectX::Keyboard::Keys::X, "X" },
            { DirectX::Keyboard::Keys::Y, "Y" },
            { DirectX::Keyboard::Keys::Z, "Z" },
            
            // 숫자
            { DirectX::Keyboard::Keys::D0, "0" },
            { DirectX::Keyboard::Keys::D1, "1" },
            { DirectX::Keyboard::Keys::D2, "2" },
            { DirectX::Keyboard::Keys::D3, "3" },
            { DirectX::Keyboard::Keys::D4, "4" },
            { DirectX::Keyboard::Keys::D5, "5" },
            { DirectX::Keyboard::Keys::D6, "6" },
            { DirectX::Keyboard::Keys::D7, "7" },
            { DirectX::Keyboard::Keys::D8, "8" },
            { DirectX::Keyboard::Keys::D9, "9" },
            
            // 화살표
            { DirectX::Keyboard::Keys::Up, "Up" },
            { DirectX::Keyboard::Keys::Down, "Down" },
            { DirectX::Keyboard::Keys::Left, "Left" },
            { DirectX::Keyboard::Keys::Right, "Right" },
            
            // 기능키
            { DirectX::Keyboard::Keys::F1, "F1" },
            { DirectX::Keyboard::Keys::F2, "F2" },
            { DirectX::Keyboard::Keys::F3, "F3" },
            { DirectX::Keyboard::Keys::F4, "F4" },
            { DirectX::Keyboard::Keys::F5, "F5" },
            { DirectX::Keyboard::Keys::F6, "F6" },
            { DirectX::Keyboard::Keys::F7, "F7" },
            { DirectX::Keyboard::Keys::F8, "F8" },
            { DirectX::Keyboard::Keys::F9, "F9" },
            { DirectX::Keyboard::Keys::F10, "F10" },
            { DirectX::Keyboard::Keys::F11, "F11" },
            { DirectX::Keyboard::Keys::F12, "F12" },
            
            // 특수키
            { DirectX::Keyboard::Keys::Space, "Space" },
            { DirectX::Keyboard::Keys::Enter, "Enter" },
            { DirectX::Keyboard::Keys::Escape, "Escape" },
            { DirectX::Keyboard::Keys::Tab, "Tab" },
            { DirectX::Keyboard::Keys::Back, "Backspace" },
            { DirectX::Keyboard::Keys::Delete, "Delete" },
            { DirectX::Keyboard::Keys::Insert, "Insert" },
            { DirectX::Keyboard::Keys::Home, "Home" },
            { DirectX::Keyboard::Keys::End, "End" },
            { DirectX::Keyboard::Keys::PageUp, "PageUp" },
            { DirectX::Keyboard::Keys::PageDown, "PageDown" },
            
            // 수정자 키
            { DirectX::Keyboard::Keys::LeftShift, "LeftShift" },
            { DirectX::Keyboard::Keys::RightShift, "RightShift" },
            { DirectX::Keyboard::Keys::LeftControl, "LeftCtrl" },
            { DirectX::Keyboard::Keys::RightControl, "RightCtrl" },
            { DirectX::Keyboard::Keys::LeftAlt, "LeftAlt" },
            { DirectX::Keyboard::Keys::RightAlt, "RightAlt" },
            
            // 넘패드
            { DirectX::Keyboard::Keys::NumPad0, "NumPad0" },
            { DirectX::Keyboard::Keys::NumPad1, "NumPad1" },
            { DirectX::Keyboard::Keys::NumPad2, "NumPad2" },
            { DirectX::Keyboard::Keys::NumPad3, "NumPad3" },
            { DirectX::Keyboard::Keys::NumPad4, "NumPad4" },
            { DirectX::Keyboard::Keys::NumPad5, "NumPad5" },
            { DirectX::Keyboard::Keys::NumPad6, "NumPad6" },
            { DirectX::Keyboard::Keys::NumPad7, "NumPad7" },
            { DirectX::Keyboard::Keys::NumPad8, "NumPad8" },
            { DirectX::Keyboard::Keys::NumPad9, "NumPad9" },
        };

        const char* KeyToString(DirectX::Keyboard::Keys key)
        {
            for (const auto& [k, name] : s_keyList)
            {
                if (k == key)
                {
                    return name;
                }
            }
            return "Unknown";
        }

        DirectX::Keyboard::Keys StringToKey(const std::string& str)
        {
            for (const auto& [key, name] : s_keyList)
            {
                if (str == name)
                {
                    return key;
                }
            }
            return DirectX::Keyboard::Keys::None;
        }

        const std::vector<std::pair<DirectX::Keyboard::Keys, const char*>>& GetAllKeys()
        {
            return s_keyList;
        }
    }

    void InputBinding::Start()
    {
        RebuildNameIndex();
    }

    void InputBinding::Update()
    {
        UpdateInputStates();
    }

    void InputBinding::UpdateInputStates()
    {
        for (auto& binding : m_bindings)
        {
            if (binding.key == DirectX::Keyboard::Keys::None)
            {
                binding.state = InputState::None;
                continue;
            }

            if (engine::Input::IsKeyPressed(binding.key))
            {
                binding.state = InputState::Pressed;
            }
            else if (engine::Input::IsKeyReleased(binding.key))
            {
                binding.state = InputState::Released;
            }
            else if (engine::Input::IsKeyHeld(binding.key))
            {
                binding.state = InputState::Held;
            }
            else
            {
                binding.state = InputState::None;
            }
        }
    }

    void InputBinding::AddBinding(const std::string& name, DirectX::Keyboard::Keys key)
    {
        if (m_nameToIndex.find(name) != m_nameToIndex.end())
        {
            LOG_PRINT("[InputBinding] Binding '{}' already exists", name);
            return;
        }

        KeyBinding binding;
        binding.name = name;
        binding.key = key;
        binding.state = InputState::None;

        m_bindings.push_back(binding);
        m_nameToIndex[name] = m_bindings.size() - 1;
    }

    void InputBinding::RemoveBinding(const std::string& name)
    {
        auto it = m_nameToIndex.find(name);
        if (it == m_nameToIndex.end())
        {
            return;
        }

        size_t index = it->second;
        m_bindings.erase(m_bindings.begin() + index);
        RebuildNameIndex();
    }

    void InputBinding::SetKey(const std::string& name, DirectX::Keyboard::Keys key)
    {
        auto it = m_nameToIndex.find(name);
        if (it != m_nameToIndex.end())
        {
            m_bindings[it->second].key = key;
        }
    }

    void InputBinding::RenameBinding(const std::string& oldName, const std::string& newName)
    {
        if (oldName == newName)
        {
            return;
        }

        auto it = m_nameToIndex.find(oldName);
        if (it == m_nameToIndex.end())
        {
            return;
        }

        if (m_nameToIndex.find(newName) != m_nameToIndex.end())
        {
            LOG_PRINT("[InputBinding] Binding '{}' already exists", newName);
            return;
        }

        size_t index = it->second;
        m_bindings[index].name = newName;
        RebuildNameIndex();
    }

    InputState InputBinding::GetInputState(const std::string& name) const
    {
        auto it = m_nameToIndex.find(name);
        if (it != m_nameToIndex.end())
        {
            return m_bindings[it->second].state;
        }
        return InputState::None;
    }

    bool InputBinding::IsPressed(const std::string& name) const
    {
        return GetInputState(name) == InputState::Pressed;
    }

    bool InputBinding::IsHeld(const std::string& name) const
    {
        auto state = GetInputState(name);
        return state == InputState::Held || state == InputState::Pressed;
    }

    bool InputBinding::IsReleased(const std::string& name) const
    {
        return GetInputState(name) == InputState::Released;
    }

    std::vector<std::string> InputBinding::GetPressedBindings() const
    {
        std::vector<std::string> result;
        for (const auto& binding : m_bindings)
        {
            if (binding.state == InputState::Pressed)
            {
                result.push_back(binding.name);
            }
        }
        return result;
    }

    std::vector<std::string> InputBinding::GetHeldBindings() const
    {
        std::vector<std::string> result;
        for (const auto& binding : m_bindings)
        {
            if (binding.state == InputState::Held || binding.state == InputState::Pressed)
            {
                result.push_back(binding.name);
            }
        }
        return result;
    }

    std::vector<std::string> InputBinding::GetReleasedBindings() const
    {
        std::vector<std::string> result;
        for (const auto& binding : m_bindings)
        {
            if (binding.state == InputState::Released)
            {
                result.push_back(binding.name);
            }
        }
        return result;
    }

    void InputBinding::OnGui()
    {
        if (ImGui::Button("Add Binding"))
        {
            std::string newName = "Action" + std::to_string(m_bindings.size());
            AddBinding(newName);
        }

        ImGui::Separator();

        std::string removeTarget;
        std::vector<std::pair<std::string, std::string>> renameList;

        for (size_t i = 0; i < m_bindings.size(); ++i)
        {
            auto& binding = m_bindings[i];
            ImGui::PushID(static_cast<int>(i));

            // 이름 편집
            char nameBuf[64];
            strcpy_s(nameBuf, binding.name.c_str());
            ImGui::SetNextItemWidth(120);
            if (ImGui::InputText("##Name", nameBuf, 64, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                if (binding.name != nameBuf)
                {
                    renameList.push_back({ binding.name, nameBuf });
                }
            }

            ImGui::SameLine();

            // 키 선택 콤보박스
            const auto& allKeys = KeyUtils::GetAllKeys();
            const char* currentKeyName = KeyUtils::KeyToString(binding.key);
            
            ImGui::SetNextItemWidth(100);
            if (ImGui::BeginCombo("##Key", currentKeyName))
            {
                for (const auto& [key, name] : allKeys)
                {
                    bool isSelected = (binding.key == key);
                    if (ImGui::Selectable(name, isSelected))
                    {
                        binding.key = key;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();

            // 현재 상태 표시
            const char* stateStr = "None";
            ImVec4 stateColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            switch (binding.state)
            {
            case InputState::Pressed:
                stateStr = "Pressed";
                stateColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                break;
            case InputState::Held:
                stateStr = "Held";
                stateColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                break;
            case InputState::Released:
                stateStr = "Released";
                stateColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
                break;
            default:
                break;
            }
            ImGui::TextColored(stateColor, "[%s]", stateStr);

            ImGui::SameLine();

            // 삭제 버튼
            if (ImGui::Button("X"))
            {
                removeTarget = binding.name;
            }

            ImGui::PopID();
        }

        // 이름 변경 처리
        for (const auto& [oldName, newName] : renameList)
        {
            RenameBinding(oldName, newName);
        }

        // 삭제 처리
        if (!removeTarget.empty())
        {
            RemoveBinding(removeTarget);
        }
    }

    void InputBinding::Save(engine::json& j) const
    {
        Object::Save(j);

        std::vector<engine::json> bindingList;
        for (const auto& binding : m_bindings)
        {
            engine::json node;
            node["Name"] = binding.name;
            node["Key"] = KeyUtils::KeyToString(binding.key);
            bindingList.push_back(node);
        }
        j["Bindings"] = bindingList;
    }

    void InputBinding::Load(const engine::json& j)
    {
        Object::Load(j);

        m_bindings.clear();
        m_nameToIndex.clear();

        engine::JsonArrayForEach(j, "Bindings", [&](const engine::json& node)
            {
                std::string name = node.value("Name", "");
                std::string keyStr = node.value("Key", "None");
                DirectX::Keyboard::Keys key = KeyUtils::StringToKey(keyStr);
                AddBinding(name, key);
            }
        );
    }

    std::string InputBinding::GetType() const
    {
        return "InputBinding";
    }

    void InputBinding::RebuildNameIndex()
    {
        m_nameToIndex.clear();
        for (size_t i = 0; i < m_bindings.size(); ++i)
        {
            m_nameToIndex[m_bindings[i].name] = i;
        }
    }
}
