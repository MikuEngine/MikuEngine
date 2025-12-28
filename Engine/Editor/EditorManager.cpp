#include "pch.h"
#include "EditorManager.h"

#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/Component/ComponentFactory.h"

#include "Editor/EditorCamera.h"

namespace engine
{
    EditorManager::EditorManager() = default;
    EditorManager::~EditorManager() = default;

    void EditorManager::Initialize()
    {
        m_editorCamera = std::make_unique<EditorCamera>();
    }

    void EditorManager::Update()
    {
        m_editorCamera->Update();
    }

    void EditorManager::Render()
    {
        auto& graphics = GraphicsDevice::Get();

        graphics.BeginDrawGUIPass();
        {
            DrawEditorController();
            DrawHierarchy();
            DrawInspector();
        }
        graphics.EndDrawGUIPass();
    }

    GameObject* EditorManager::GetSelectedObject() const
    {
        return nullptr;
    }

    EditorState EditorManager::GetEditorState() const
    {
        return m_editorState;
    }

    EditorCamera* EditorManager::GetEditorCamera() const
    {
        return m_editorCamera.get();
    }

    void EditorManager::SetSelectedObject(GameObject* gameObject)
    {
    }

    void EditorManager::DrawEditorController()
    {
        ImGui::Begin("Editor Control");
        if (ImGui::Button("Save Scene"))
        {
            SceneManager::Get().GetCurrentScene()->Save("Resource/Scene/scene.json");
        }

        ImGui::SameLine();

        if (ImGui::Button("Load Scene"))
        {
            SceneManager::Get().GetCurrentScene()->Load("Resource/Scene/scene.json");
            m_selectedObject = nullptr;
        }

        m_editorCamera->OnGui();
        ImGui::End();
    }

    void EditorManager::DrawHierarchy()
    {
        ImGui::Begin("Hierarchy");

        if (ImGui::Button("Create GameObject"))
        {
            SceneManager::Get().GetCurrentScene()->CreateGameObject();
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

        auto scene = SceneManager::Get().GetCurrentScene();

        if (scene != nullptr)
        {
            for (const auto& gameObject : scene->GetGameObjects())
            {
                if (gameObject->GetTransform()->GetParent() == nullptr)
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
        
        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)gameObject, flags, gameObject->GetName().c_str());

        if (ImGui::IsItemClicked())
        {
            m_selectedObject = gameObject;
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

                if (draggedObject != gameObject && !draggedObject->GetTransform()->IsDescendantOf(gameObject->GetTransform()))
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

            bool open = ImGui::CollapsingHeader(comp->GetType().c_str(), ImGuiTreeNodeFlags_DefaultOpen);

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

        ImGui::End();
    }
}