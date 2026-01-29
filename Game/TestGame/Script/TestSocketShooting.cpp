#include "GamePCH.h"
#include "TestSocketShooting.h"

#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/Component/Renderer/SkeletalMeshRenderer.h"
#include "Framework/Object/Component/Renderer/StaticMeshRenderer.h"
#include "Framework/Object/Component/Particle/ParticleEffect.h"
#include "Core/System/Input.h"

namespace game
{
    void TestSocketShooting::Awake()
    {
        auto scene = engine::SceneManager::Get().GetScene();

        if (scene)
        {
            m_player = scene->FindGameObject(m_playerName);
            m_gun = scene->FindGameObject(m_gunName);
            m_muzzleFlash = scene->FindGameObject(m_muzzleFlashName);
        }
    }

    void TestSocketShooting::Update()
    {
        if (m_player && m_gun)
        {
            auto renderer = m_player->GetComponent<engine::SkeletalMeshRenderer>();
            if (renderer)
            {
                engine::Matrix handMatrix = renderer->GetSocketWorldMatrix("Socket_LeftHand");

                // 소켓 행렬 분해 (위치와 회전만 필요)
                engine::Vector3 socketPos, socketScale;
                engine::Quaternion socketRot;
                handMatrix.Decompose(socketScale, socketRot, socketPos);

                auto gunTransform = m_gun->GetTransform();

                engine::Vector3 currentGunScale = gunTransform->GetLocalScale();
                engine::Matrix newGunWorld =
                    engine::Matrix::CreateScale(currentGunScale) * engine::Matrix::CreateFromQuaternion(socketRot) * engine::Matrix::CreateTranslation(socketPos);

                gunTransform->SetWorldMatrix(newGunWorld);

                auto gunRenderer = m_gun->GetComponent<engine::StaticMeshRenderer>();
                if (gunRenderer) gunRenderer->UpdateSockets();
            }
        }

        if (m_gun && m_muzzleFlash)
        {
            auto renderer = m_gun->GetComponent<engine::StaticMeshRenderer>();
            if (renderer)
            {
                engine::Matrix muzzleMatrix = renderer->GetSocketWorldMatrix("socket_gun_fire");
                m_muzzleFlash->GetTransform()->SetWorldMatrix(muzzleMatrix);
            }
        }

        if (engine::Input::IsKeyPressed(engine::Keys::Space))
        {
            if (m_muzzleFlash)
            {
                auto particle = m_muzzleFlash->GetComponent<engine::ParticleEffect>();
                if (particle)
                {
                    m_muzzleFlash->SetActive(true);
                    particle->Play();
                }
            }
        }

        if (engine::Input::IsKeyReleased(engine::Keys::Space))
        {
            if (m_muzzleFlash)
            {
                auto particle = m_muzzleFlash->GetComponent<engine::ParticleEffect>();
                if (particle)
                {
                    particle->Stop();
                }
            }
        }
    }

    void TestSocketShooting::OnGui()
    {
        ImGui::Text("Attachment Settings (By Name)");

        char pName[64], gName[64], mName[64];
        strcpy_s(pName, m_playerName.c_str());
        strcpy_s(gName, m_gunName.c_str());
        strcpy_s(mName, m_muzzleFlashName.c_str());

        if (ImGui::InputText("Player Name", pName, 64)) m_playerName = pName;
        if (ImGui::InputText("Gun Name", gName, 64)) m_gunName = gName;
        if (ImGui::InputText("Muzzle Name", mName, 64)) m_muzzleFlashName = mName;

        if (ImGui::Button("Link Objects"))
        {
            Awake(); 
        }
    }

    void TestSocketShooting::Save(engine::json& j) const
    {
        engine::Object::Save(j);
        j["PlayerName"] = m_playerName;
        j["GunName"] = m_gunName;
        j["MuzzleFlashName"] = m_muzzleFlashName;
    }

    void TestSocketShooting::Load(const engine::json& j)
    {
        engine::Object::Load(j);
        m_playerName = j.value("PlayerName", "Player");
        m_gunName = j.value("GunName", "GunObject");
        m_muzzleFlashName = j.value("MuzzleFlashName", "ParticleObject");
    }
}