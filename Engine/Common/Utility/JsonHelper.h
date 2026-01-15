#pragma once

#include <json.hpp>
#include <directxtk/SimpleMath.h>

namespace DirectX::SimpleMath
{
    inline void to_json(nlohmann::ordered_json& j, const Vector2& v)
    {
        j = nlohmann::ordered_json{
            { "x", v.x },
            { "y", v.y }
        };
    }

    inline void from_json(const nlohmann::ordered_json& j, Vector2& v)
    {
        v.x = j.at("x");
        v.y = j.at("y");
    }

    inline void to_json(nlohmann::ordered_json& j, const Vector3& v)
    {
        j = nlohmann::ordered_json{
            { "x", v.x },
            { "y", v.y },
            { "z", v.z }
        };
    }

    inline void from_json(const nlohmann::ordered_json& j, Vector3& v)
    {
        v.x = j.at("x");
        v.y = j.at("y");
        v.z = j.at("z");
    }

    inline void to_json(nlohmann::ordered_json& j, const Vector4& v)
    {
        j = nlohmann::ordered_json{
            { "x", v.x },
            { "y", v.y },
            { "z", v.z },
            { "w", v.w }
        };
    }

    inline void from_json(const nlohmann::ordered_json& j, Vector4& v)
    {
        v.x = j.at("x");
        v.y = j.at("y");
        v.z = j.at("z");
        v.w = j.at("w");
    }

    inline void to_json(nlohmann::ordered_json& j, const Quaternion& q)
    {
        j = nlohmann::ordered_json{
            { "x", q.x },
            { "y", q.y },
            { "z", q.z },
            { "w", q.w }
        };
    }

    inline void from_json(const nlohmann::ordered_json& j, Quaternion& q)
    {
        q.x = j.at("x");
        q.y = j.at("y");
        q.z = j.at("z");
        q.w = j.at("w");
    }
}

namespace engine
{
    template <typename T>
    inline void JsonGet(const nlohmann::ordered_json& j, const std::string& key, T& outValue)
    {
        if (auto iter = j.find(key); iter != j.end())
        {
            outValue = iter.value();
        }
    }

    template <typename Func>
    inline void JsonArrayForEach(const nlohmann::ordered_json& j, const std::string& key, Func func)
    {
        if (auto iter = j.find(key); iter != j.end() && iter->is_array())
        {
            for (const auto& item : iter.value())
            {
                func(item);
            }
        }
    }

    inline bool DrawFileSelector(const char* label, const std::string& basePath, const std::string& extension, std::string& outSelectedPath)
    {
        namespace fs = std::filesystem;
        bool result = false;
        if (ImGui::Button(label))
        {
            ImGui::OpenPopup(label);
        }

        // 팝업 ID가 겹치지 않게 label 사용
        if (ImGui::BeginPopupModal(label, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::Separator();

            ImGui::BeginChild("FileList", ImVec2(400, 300), true);
            if (fs::exists(basePath))
            {
                for (const auto& entry : fs::recursive_directory_iterator(basePath))
                {
                    if (entry.is_regular_file())
                    {
                        // 확장자 체크
                        if (entry.path().extension() == extension)
                        {
                            std::string fullPath = entry.path().generic_string();
                            // 선택 가능 항목
                            if (ImGui::Selectable(fullPath.c_str()))
                            {
                                outSelectedPath = fullPath;
                                result = true;
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Path not found: %s", basePath.c_str());
            }
            ImGui::EndChild();
            ImGui::EndPopup();
        }
        return result;
    }

    inline bool DrawFileSelector(const char* label, const std::string& basePath, const std::vector<std::string>& extensions, std::string& outSelectedPath)
    {
        namespace fs = std::filesystem;
        bool result = false;
        if (ImGui::Button(label))
        {
            ImGui::OpenPopup(label);
        }

        // 팝업 ID가 겹치지 않게 label 사용
        if (ImGui::BeginPopupModal(label, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::Separator();

            ImGui::BeginChild("FileList", ImVec2(400, 300), true);
            if (fs::exists(basePath))
            {
                for (const auto& entry : fs::recursive_directory_iterator(basePath))
                {
                    if (entry.is_regular_file())
                    {
                        // 확장자 체크
                        if (std::find(extensions.begin(), extensions.end(), entry.path().extension()) != extensions.end())
                        {
                            std::string fullPath = entry.path().generic_string();
                            // 선택 가능 항목
                            if (ImGui::Selectable(fullPath.c_str()))
                            {
                                outSelectedPath = fullPath;
                                result = true;
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Path not found: %s", basePath.c_str());
            }
            ImGui::EndChild();
            ImGui::EndPopup();
        }
        return result;
    }
}