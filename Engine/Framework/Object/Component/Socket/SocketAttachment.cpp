#include "EnginePCH.h"
#include "SocketAttachment.h"
#include "Framework/Object/Component/Renderer/Renderer.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Transform.h"

namespace engine
{
    void SocketAttachment::Update()
    {
        if (m_targetRenderer && !m_socketName.empty())
        {
            Matrix socketMatrix = m_targetRenderer->GetSocketWorldMatrix(m_socketName);
            GetTransform()->SetWorldMatrix(socketMatrix);
        }
    }

    void SocketAttachment::SetSocket(Renderer* renderer, const std::string& socketName)
    {
        m_targetRenderer = renderer;
        m_socketName = socketName;
    }

    void SocketAttachment::OnGui()
    {
        Script::OnGui();
        ImGui::Text("Target Renderer: %s", m_targetRenderer ? m_targetRenderer->GetGameObject()->GetName().c_str() : "None");

        char buffer[256];
        strcpy_s(buffer, m_socketName.c_str());
        if (ImGui::InputText("Socket Name", buffer, sizeof(buffer)))
        {
            m_socketName = buffer;
        }
    }

    void SocketAttachment::Save(json& j) const
    {
        Script::Save(j);

        j["SocketName"] = m_socketName;
    }

    void SocketAttachment::Load(const json& j)
    {
        Script::Load(j);

        JsonGet(j, "SocketName", m_socketName);
    }
}