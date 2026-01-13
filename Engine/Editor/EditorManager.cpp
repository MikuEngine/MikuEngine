#include "EnginePCH.h"
#include "EditorManager.h"

#include <fstream>

#include "Common/Utility/Profiling.h"
#include "Common/Utility/StringHelper.h"
#include "Core/Graphics/Device/GraphicsDevice.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/Component/ComponentFactory.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"

#include "Editor/EditorCamera.h"

#include "Framework/Physics/PhysicsDebugRenderer.h"
#include "Framework/Physics/PhysicsSystem.h"
#include "Framework/Physics/CollisionSystem.h"

namespace engine
{
    namespace
    {
        nlohmann::ordered_json g_tempScene;
    }

    EditorManager::EditorManager() = default;
    EditorManager::~EditorManager() = default;

    void EditorManager::Initialize()
    {
        m_editorCamera = std::make_unique<EditorCamera>();
        m_projectSettings.Load();

        RefreshFileCache();
        ValidateSettingsList();

        bool sceneLoaded = false;

        if (!m_projectSettings.sceneList.empty())
        {
            std::string path = "Resource/Scene/" + m_projectSettings.sceneList[0] + ".json";
            if (std::filesystem::exists(path))
            {
                SceneManager::Get().ChangeScene(m_projectSettings.sceneList[0]);
                sceneLoaded = true;
            }
        }

        if (!sceneLoaded && !m_cachedSceneFiles.empty())
        {
            SceneManager::Get().ChangeScene(m_cachedSceneFiles[0]);
            sceneLoaded = true;
        }

        if (!sceneLoaded)
        {
            SceneManager::Get().GetScene()->Save();
            RefreshFileCache();
        }
    }

    void EditorManager::Update()
    {
        if (!ImGui::GetIO().WantCaptureMouse)
        {
            m_editorCamera->Update();
        }
    }

    void EditorManager::Render()
    {
        auto& graphics = GraphicsDevice::Get();

        graphics.BeginDrawGUIPass();
        {
            DrawPlayController();
            DrawEditorController();
            DrawHierarchy();
            DrawInspector();
            DrawDebugInfo();
        }
        graphics.EndDrawGUIPass();
    }

    EditorState EditorManager::GetEditorState() const
    {
        return m_editorState;
    }

    EditorCamera* EditorManager::GetEditorCamera() const
    {
        return m_editorCamera.get();
    }

    void EditorManager::DrawPlayController()
    {
        ImGui::Begin("Play Control");

        if (m_editorState == EditorState::Edit)
        {
            if (ImGui::Button("Play"))
            {
                auto scene = SceneManager::Get().GetScene();
                g_tempScene.clear();
                scene->SaveToJson(g_tempScene);
                scene->LoadFromJson(g_tempScene);

                // 물리 씬 생성
                PhysicsSystem::Get().CreateScenePhysics(scene);

                m_editorState = EditorState::Play;

                m_selectedObject = nullptr;
            }
        }
        else
        {
            if (ImGui::Button("Stop"))
            {
                auto scene = SceneManager::Get().GetScene();
                
                // 물리 씬 파괴
                PhysicsSystem::Get().DestroyScenePhysics(scene);
                
                if (scene && !g_tempScene.empty())
                {
                    scene->LoadFromJson(g_tempScene);
                }
                m_editorState = EditorState::Edit;

                m_selectedObject = nullptr;
            }
        }

        ImGui::SameLine();

        if (m_editorState == EditorState::Pause)
        {
            if (ImGui::Button("Resume"))
            {
                m_editorState = EditorState::Play;
            }
        }
        else if (m_editorState == EditorState::Play)
        {
            if (ImGui::Button("Pause"))
            {
                m_editorState = EditorState::Pause;
            }
        }

        ImGui::End();
    }

    void EditorManager::DrawEditorController()
    {
        ImGui::Begin("Editor Control");

        auto currentScene = SceneManager::Get().GetScene();

        if (ImGui::CollapsingHeader("Project Settings", ImGuiTreeNodeFlags_CollapsingHeader))
        {
            ImGui::Text("Build Scene List");
            if (ImGui::BeginListBox("##BuildSceneList", ImVec2(-1, 100)))
            {
                for (int i = 0; i < static_cast<int>(m_projectSettings.sceneList.size()); ++i)
                {
                    bool isSelected = (m_selectedBuildSceneIndex == i);
                    std::string label = m_projectSettings.sceneList[i];

                    if (ImGui::Selectable(label.c_str(), isSelected))
                    {
                        m_selectedBuildSceneIndex = i;
                    }

                    if (ImGui::BeginDragDropSource())
                    {
                        ImGui::SetDragDropPayload("SCENE_ORDER", &i, sizeof(int));
                        ImGui::Text(label.c_str());
                        ImGui::EndDragDropSource();
                    }

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ORDER"))
                        {
                            int fromIdx = *(int*)payload->Data;
                            int toIdx = i;
                            if (fromIdx != toIdx)
                            {
                                std::string temp = m_projectSettings.sceneList[fromIdx];
                                m_projectSettings.sceneList.erase(m_projectSettings.sceneList.begin() + fromIdx);
                                m_projectSettings.sceneList.insert(m_projectSettings.sceneList.begin() + toIdx, temp);
                                m_selectedBuildSceneIndex = toIdx;
                                m_projectSettings.Save();
                            }
                        }

                        ImGui::EndDragDropTarget();
                    }

                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::MenuItem("Remove"))
                        {
                            m_projectSettings.sceneList.erase(m_projectSettings.sceneList.begin() + i);
                            m_projectSettings.Save();

                            ImGui::EndPopup();
                            break;
                        }
                        ImGui::EndPopup();
                    }
                }
            }

            ImGui::EndListBox();

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_NAME"))
                {
                    const char* droppedName = (const char*)payload->Data;

                    bool exist = false;
                    for (const auto& str : m_projectSettings.sceneList)
                    {
                        if (str == droppedName)
                        {
                            exist = true;
                            break;
                        }
                    }

                    if (!exist)
                    {
                        m_projectSettings.sceneList.push_back(droppedName);
                        m_projectSettings.Save();
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }

        if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Current Scene: %s", currentScene->GetName().c_str());

            if (m_editorState == EditorState::Edit)
            {
                if (ImGui::Button("Save Scene"))
                {
                    currentScene->Save();
                    RefreshFileCache();
                }
            }
            else
            {
                // 플레이 중임을 알리는 비활성화된 버튼 (선택사항)
                ImGui::BeginDisabled();
                ImGui::Button("Save Disabled (Playing)");
                ImGui::EndDisabled();
            }

            ImGui::Text("Scene File List");

            if (ImGui::BeginListBox("##SceneFileList"))
            {
                for (const auto& filename : m_cachedSceneFiles)
                {
                    if (ImGui::Selectable(filename.c_str(), false))
                    {
                        // 선택 로직...
                    }

                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::MenuItem("Rename"))
                        {
                            // 팝업 열기 위한 플래그 혹은 변수 설정
                            m_sceneToRename = filename;
                        }

                        // "Delete" 메뉴 클릭 시
                        if (ImGui::MenuItem("Delete"))
                        {
                            m_sceneToDelete = filename; // 멤버변수에 삭제 대상 저장
                        }
                        ImGui::EndPopup();
                    }

                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                    {
                        ImGui::SetDragDropPayload("SCENE_NAME", filename.c_str(), filename.size() + 1);
                        ImGui::Text("Add %s", filename.c_str());
                        ImGui::EndDragDropSource();
                    }
                    
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && m_editorState == EditorState::Edit)
                    {
                        RequestSceneChange(filename);
                    }
                }

                ImGui::EndListBox();
            }

            if (ImGui::Button("New Scene"))
            {
                RequestNewScene();
            }

            ImGui::SameLine();

            if (ImGui::Button("Refresh"))
            {
                RefreshFileCache();
            }

            if (!m_sceneToDelete.empty())
            {
                ImGui::OpenPopup("Delete Check");
            }

            if (!m_sceneToRename.empty())
            {
                ImGui::OpenPopup("Rename Scene Popup");
            }

            if (ImGui::BeginPopupModal("Delete Check", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Are you sure you want to delete '%s'?", m_sceneToDelete.c_str());
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "This operation cannot be undone!");
                ImGui::Separator();
                if (ImGui::Button("Delete", ImVec2(120, 0)))
                {
                    // 1. 현재 열린 씬인지 확인
                    bool isToDeleteCurrent = (SceneManager::Get().GetScene()->GetName() == m_sceneToDelete);
                    // 2. 파일 삭제
                    namespace fs = std::filesystem;
                    std::string path = "Resource/Scene/" + m_sceneToDelete + ".json";
                    if (fs::exists(path))
                    {
                        fs::remove(path);
                    }
                    // 3. Project Settings에서도 제거
                    auto& list = m_projectSettings.sceneList;
                    auto it = std::find(list.begin(), list.end(), m_sceneToDelete);
                    if (it != list.end())
                    {
                        list.erase(it);
                        m_projectSettings.Save();
                    }
                    // 4. 캐시 갱신
                    RefreshFileCache();
                    // 5. ★ 현재 씬을 삭제했다면? => 대안 씬 로드
                    if (isToDeleteCurrent)
                    {
                        if (!m_cachedSceneFiles.empty())
                        {
                            // 다른 파일이 있으면 첫 번째 것 로드 (변경사항 체크 없이 강제 로드)
                            SceneManager::Get().ChangeScene(m_cachedSceneFiles[0]);
                        }
                        else
                        {
                            // 파일이 하나도 없으면 새 씬(Untitled) 생성
                            CreateNewScene();
                        }
                    }
                    m_sceneToDelete.clear();
                    ImGui::CloseCurrentPopup();
                    m_selectedObject = nullptr;
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    m_sceneToDelete.clear();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            if (m_shouldOpenUnsavedPopup)
            {
                ImGui::OpenPopup("Unsaved Changes");
                m_shouldOpenUnsavedPopup = false;
            }

            if (ImGui::BeginPopupModal("Rename Scene Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                static char renameBuf[256];
                // 팝업 열릴 때 초기화 (한 번만)
                if (ImGui::IsWindowAppearing())
                {
                    strcpy_s(renameBuf, m_sceneToRename.c_str());
                }

                ImGui::InputText("New Name", renameBuf, 256);

                if (ImGui::Button("Rename", ImVec2(120, 0)))
                {
                    namespace fs = std::filesystem;

                    std::string oldPath = "Resource/Scene/" + m_sceneToRename + ".json";
                    std::string newPath = "Resource/Scene/" + std::string(renameBuf) + ".json";

                    if (!fs::exists(newPath))
                    {
                        // 1. 파일 이름 변경 (A.json -> B.json)
                        fs::rename(oldPath, newPath);
                        // 2. 내부 데이터("Name")도 즉시 수정
                        // currentScene이든 아니든, 파일 자체를 열어서 수정해버림
                        std::ifstream i(newPath);
                        if (i.is_open())
                        {
                            nlohmann::ordered_json j;
                            i >> j;
                            i.close();
                            // 이름 필드 업데이트
                            j["Name"] = std::string(renameBuf);     // Scene 클래스의 멤버변수명("m_Name" 등)이 아니라 JSON Key("Name") 확인 필요
                            // 보통 Scene::SaveToJson에서 "Name" 키로 저장함
                            std::ofstream o(newPath);
                            o << std::setw(4) << j << std::endl;
                        }
                        // 3. 현재 열려있는 씬이라면 메모리 이름도 같이 변경 (UI 즉시 반영용)
                        auto current = SceneManager::Get().GetScene();
                        if (current->GetName() == m_sceneToRename)
                        {
                            current->SetName(renameBuf);
                        }

                        // 4. 세팅 파일이나 리스트 업데이트
                        bool settingChanged = false;
                        for (auto& sceneItem : m_projectSettings.sceneList)
                        {
                            if (sceneItem == m_sceneToRename)
                            {
                                sceneItem = renameBuf;
                                settingChanged = true;
                            }
                        }
                        if (settingChanged)
                        {
                            m_projectSettings.Save();
                        }
                        RefreshFileCache();
                        ImGui::CloseCurrentPopup();
                    }
                    m_sceneToRename.clear();
                }

                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    m_sceneToRename.clear();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            if (ImGui::BeginPopupModal("Unsaved Changes", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("You have unsaved changes.");
                ImGui::Text("Do you want to save before loading the next scene?");
                ImGui::Separator();
                if (ImGui::Button("Save & Load", ImVec2(120, 0)))
                {
                    m_selectedObject = nullptr;
                    // 저장 수행
                    SceneManager::Get().GetScene()->Save();

                    // 다음 씬 로드
                    if (m_nextScenePending == "NEW_SCENE")
                    {
                        CreateNewScene();
                    }
                    else
                    {
                        SceneManager::Get().ChangeScene(m_nextScenePending);
                    }

                    m_nextScenePending.clear();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();
                if (ImGui::Button("Discard", ImVec2(120, 0)))
                {
                    // 저장 안 하고 바로 로드
                    m_selectedObject = nullptr;

                    if (m_nextScenePending == "NEW_SCENE")
                    {
                        CreateNewScene();
                    }
                    else
                    {
                        SceneManager::Get().ChangeScene(m_nextScenePending);
                    }

                    m_nextScenePending.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    m_nextScenePending.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        if (ImGui::CollapsingHeader("Editor Camera", ImGuiTreeNodeFlags_DefaultOpen))
        {
            m_editorCamera->OnGui();
        }

        if (ImGui::CollapsingHeader("Post Processing Params", ImGuiTreeNodeFlags_DefaultOpen))
        {
            float bloomStrength;
            float bloomThreshold;
            float bloomSoftKnee;

            SystemManager::Get().GetRenderSystem().GetBloomSettings(bloomStrength, bloomThreshold, bloomSoftKnee);

            ImGui::DragFloat("Bloom Strength", &bloomStrength, 0.001f, 0.0001f, 999999.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Bloom Threshold", &bloomThreshold, 0.001f, 0.0001f, 999999.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Bloom SoftKnee", &bloomSoftKnee, 0.001f, 0.0001f, 999999.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

            SystemManager::Get().GetRenderSystem().SetBloomSettings(bloomStrength, bloomThreshold, bloomSoftKnee);
        }

        ImGui::End();
    }

    void EditorManager::DrawHierarchy()
    {
        ImGui::Begin("Hierarchy");

        if (ImGui::Button("Create GameObject"))
        {
            SceneManager::Get().GetScene()->CreateGameObject();
        }

        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 10.0f));
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_DRAG"))
            {
                GameObject* dropped = *(GameObject**)payload->Data;
                dropped->GetTransform()->SetParent(nullptr); // 부모 해제 (Root로 이동)
            }

            ImGui::EndDragDropTarget();
        }

        auto scene = SceneManager::Get().GetScene();

        if (scene != nullptr)
        {
            for (const auto& gameObject : scene->GetGameObjects())
            {
                if (gameObject && gameObject->GetTransform()->GetParent() == nullptr)
                {
                    DrawEntityNode(gameObject.get());
                }
            }
        }

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
        {
            m_selectedObject = nullptr;
        }

        ImGui::End();
    }

    void EditorManager::DrawEntityNode(GameObject* gameObject)
    {
        ImGuiTreeNodeFlags flags = ((m_selectedObject == gameObject) ? ImGuiTreeNodeFlags_Selected : 0);
        flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (gameObject->GetTransform()->GetChildren().empty())
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        
        // [추가] 비활성화 시 회색 처리 (Alpha 0.5)
        bool isHierarchyActive = gameObject->IsActive(); // IsActive()는 부모까지 체크함
        if (!isHierarchyActive)
        {
            ImVec4 v4 = ImGui::GetStyle().Colors[ImGuiCol_Text];
            v4.w = 0.5f;
            ImGui::PushStyleColor(ImGuiCol_Text, v4);
        }

        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)gameObject, flags, gameObject->GetName().c_str());

        if (!isHierarchyActive)
        {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemClicked())
        {
            m_selectedObject = gameObject;
        }

        if (ImGui::BeginPopupContextItem())
        {
            // 여기서 바로 삭제 (Delete 키 확인 로직 없이 심플하게 메뉴만)
            if (ImGui::MenuItem("Delete"))
            {
                if (m_selectedObject == gameObject)
                {
                    m_selectedObject = nullptr;
                }

                // 2. 에디터 전용 삭제 함수 호출 (선형 탐색 + 즉시 삭제)
                SceneManager::Get().GetScene()->RemoveGameObjectEditor(gameObject);

                // 삭제 후 바로 팝업 닫고 리턴 (더 그리면 유효하지 않은 포인터 접근 위험)
                ImGui::EndPopup();
                if (opened)
                {
                    ImGui::TreePop();
                }

                return;
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("HIERARCHY_DRAG", &gameObject, sizeof(GameObject*));
            ImGui::Text(gameObject->GetName().c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_DRAG"))
            {
                GameObject* draggedObject = *(GameObject**)payload->Data;

                if (draggedObject != gameObject && !draggedObject->GetTransform()->IsAncestorOf(gameObject->GetTransform()))
                {
                    draggedObject->GetTransform()->SetParent(gameObject->GetTransform());
                }
            }

            ImGui::EndDragDropTarget();
        }
        
        if (opened)
        {
            for (auto child : gameObject->GetTransform()->GetChildren())
            {
                DrawEntityNode(child->GetGameObject());
            }

            ImGui::TreePop();
        }
    }

    void EditorManager::DrawInspector()
    {
        ImGui::Begin("Inspector");

        if (!m_selectedObject)
        {
            ImGui::End();
            return;
        }
        
        ImGui::PushID(m_selectedObject);

        bool goActive = m_selectedObject->IsActiveSelf();
        if (ImGui::Checkbox("##GO_Active", &goActive))
        {
            m_selectedObject->SetActive(goActive);
        }
        ImGui::SameLine();

        char buf[256];
        strcpy_s(buf, m_selectedObject->GetName().c_str());
        if (ImGui::InputText("Name", buf, 256))
        {
            m_selectedObject->SetName(buf);
        }
        
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            m_selectedObject->GetTransform()->OnGui();
        }
        
        ImGui::Spacing();

        auto& components = m_selectedObject->GetComponents();
        int removeIndex = -1;
        for (int i = 0; i < components.size(); ++i)
        {
            auto& comp = components[i];
            ImGui::PushID(comp.get());

            if (comp->GetType() == "Transform")
            {
                ImGui::PopID();
                continue;
            }

            // [추가] Component Active Checkbox
            // 중요: Object::IsActive()를 명시적 호출하여 순수하게 멤버 변수 m_active 값만 가져옴
            bool compActive = comp->Object::IsActive();
            if (ImGui::Checkbox("##Comp_Active", &compActive))
            {
                comp->SetActive(compActive);
            }
            ImGui::SameLine(); // 헤더와 한 줄에 표시하기 위함

            bool isCompActive = comp->Object::IsActive(); // 단순히 체크박스 상태만 반영
            if (!isCompActive)
            {
                ImVec4 v4 = ImGui::GetStyle().Colors[ImGuiCol_Text];
                v4.w = 0.5f;
                ImGui::PushStyleColor(ImGuiCol_Text, v4);
            }

            bool open = ImGui::CollapsingHeader(comp->GetType().c_str(), ImGuiTreeNodeFlags_DefaultOpen);

            if (!isCompActive)
            {
                ImGui::PopStyleColor();
            }

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Remove Component"))
                {
                    removeIndex = i;
                }

                ImGui::EndPopup();
            }

            if (open)
            {
                comp->OnGui();
            }

            ImGui::PopID();

            ImGui::Spacing();
        }
        
        if (removeIndex != -1)
        {
            m_selectedObject->RemoveComponent(static_cast<size_t>(removeIndex));
        }

        ImGui::Separator();

        if (ImGui::Button("Add Component", ImVec2(-1, 0)))
        {
            ImGui::OpenPopup("AddCompPopup");
        }

        if (ImGui::BeginPopup("AddCompPopup"))
        {
            for (const auto& [name, creator] : ComponentFactory::Get().GetRegistry())
            {
                if (ImGui::MenuItem(name.c_str()))
                {
                    m_selectedObject->AddComponent(std::move(creator()));
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
        ImGui::End();
    }

    void EditorManager::DrawDebugInfo()
    {
        auto bufferTextures = GraphicsDevice::Get().GetBufferTextures();

        ImGui::Begin("Debug Info");

        if (ImGui::CollapsingHeader("Buffer Textures", ImGuiTreeNodeFlags_CollapsingHeader))
        {
            ImGui::BeginGroup();
            ImGui::Text("Post Process");
            ImGui::Image((ImTextureID)bufferTextures.hdr, ImVec2(128, 128));
            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::BeginGroup();
            ImGui::Text("Game Depth");
            ImGui::Image((ImTextureID)bufferTextures.gameDepth, ImVec2(128, 128));
            ImGui::EndGroup();

            ImGui::BeginGroup();
            ImGui::Text("Shadow Depth");
            ImGui::Image((ImTextureID)bufferTextures.shadowDepth, ImVec2(128, 128));
            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::BeginGroup();
            ImGui::Text("Base Color");
            ImGui::Image((ImTextureID)bufferTextures.baseColor, ImVec2(128, 128));
            ImGui::EndGroup();

            ImGui::BeginGroup();
            ImGui::Text("Normal");
            ImGui::Image((ImTextureID)bufferTextures.normal, ImVec2(128, 128));
            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::BeginGroup();
            ImGui::Text("AO/Roughness/Metalness");
            ImGui::Image((ImTextureID)bufferTextures.orm, ImVec2(128, 128));
            ImGui::EndGroup();

            ImGui::BeginGroup();
            ImGui::Text("Emissive");
            ImGui::Image((ImTextureID)bufferTextures.emissive, ImVec2(128, 128));
            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::BeginGroup();
            ImGui::Text("Bloom Half");
            ImGui::Image((ImTextureID)bufferTextures.bloomHalfBuffer, ImVec2(128, 128));
            ImGui::EndGroup();

            ImGui::BeginGroup();
            ImGui::Text("Bloom Work");
            ImGui::Image((ImTextureID)bufferTextures.bloomWorkBuffer, ImVec2(128, 128));
            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::BeginGroup();
            ImGui::Text("FXAA");
            ImGui::Image((ImTextureID)bufferTextures.aaBuffer, ImVec2(128, 128));
            ImGui::EndGroup();
        }

        if (ImGui::CollapsingHeader("Profile", ImGuiTreeNodeFlags_CollapsingHeader))
        {
            ImGui::Text("FPS: %d", Profiling::GetLastFPS());
            ImGui::Text("DRAM: %s", FormatBytes(Profiling::GetDRAMUsage()).c_str());
            ImGui::Text("VRAM: %s", FormatBytes(Profiling::GetVRAMUsage()).c_str());
            ImGui::Text("PageFile: %s", FormatBytes(Profiling::GetPageFileUsage()).c_str());
        }

        // Physics Debug
        PhysicsDebugRenderer::Get().OnGui();

        ImGui::End();
    }

    void EditorManager::ValidateSettingsList()
    {
        namespace fs = std::filesystem;

        auto& list = m_projectSettings.sceneList;
        bool dirty = false;

        for (auto it = list.begin(); it != list.end(); )
        {
            std::string fullPath = "Resource/Scene/" + *it + ".json";
            if (!fs::exists(fullPath))
            {
                it = list.erase(it);
                dirty = true;
            }
            else
            {
                ++it;
            }
        }

        if (dirty)
        {
            m_projectSettings.Save();
        }
    }

    void EditorManager::RefreshFileCache()
    {
        namespace fs = std::filesystem;

        m_cachedSceneFiles.clear();

        std::string dirPath = "Resource/Scene";

        if (!fs::exists(dirPath))
        {
            fs::create_directories(dirPath);
        }

        for (const auto& entry : fs::directory_iterator(dirPath))
        {
            if (entry.path().extension() == ".json")
            {
                m_cachedSceneFiles.push_back(entry.path().filename().replace_extension().string());
            }
        }
    }

    bool EditorManager::IsSceneDirty()
    {
        // 현재 메모리 상의 씬 데이터 (JSON)
        json currentJson;

        SceneManager::Get().GetScene()->SaveToJson(currentJson);

        std::string currentStr = currentJson.dump();
        // 디스크에 있는 파일 데이터 load
        std::string path = "Resource/Scene/" + SceneManager::Get().GetScene()->GetName() + ".json";

        if (!std::filesystem::exists(path))
        {
            return true; // 파일 없으면 변경(생성)된 것
        }

        std::ifstream i(path);
        json savedJson;
        i >> savedJson;

        std::string savedStr = savedJson.dump();
        
        return currentStr != savedStr;
    }

    void EditorManager::RequestSceneChange(const std::string& nextSceneName)
    {
        if (IsSceneDirty())
        {
            m_nextScenePending = nextSceneName; // 가려던 곳 저장
            //ImGui::OpenPopup("Unsaved Changes");
            m_shouldOpenUnsavedPopup = true;
        }
        else
        {
            SceneManager::Get().ChangeScene(nextSceneName);
            m_selectedObject = nullptr;
        }
    }
    void EditorManager::RequestNewScene()
    {
        // 변경사항이 있으면 팝업 띄우기
        if (IsSceneDirty())
        {
            m_nextScenePending = "NEW_SCENE"; // 특수 플래그 문자열 사용
            m_shouldOpenUnsavedPopup = true;
        }
        else
        {
            // 깨끗하면 바로 생성
            CreateNewScene();
        }
    }
    void EditorManager::CreateNewScene()
    {
        SceneManager::Get().GetScene()->ResetToDefaultScene();
        SceneManager::Get().GetScene()->SetName("Untitled"); // 저장 안 된 상태
        // 선택된 오브젝트 해제
        m_selectedObject = nullptr;
    }
}