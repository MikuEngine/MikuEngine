#include "GamePCH.h"
#include "PlayerAimMeshController.h"

#include "Script/AimPointer.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/AnimFSM.h>
#include <Framework/Object/Component/LogicFSM.h>
#include <Engine/Core/System/Input.h>
#include <Engine/Framework/Object/Component/Renderer/AfterimageRenderer.h>

#include "Script/CharacterScript/Player/PlayerControllerScript.h"

namespace game
{
    void PlayerAimMeshController::Start()
    {
        CacheReferences();
        
        auto comp = GetGameObject()->GetComponent<engine::AfterimageRenderer>();
        m_playerObject->GetComponent<PlayerControllerScript>()->SetAfterImage(comp);

        // AnimFSM 초기화 (상태 등록)
        if (m_animFSM && !m_animFSMInitialized)
        {
            InitializeAnimFSM();
            m_animFSMInitialized = true;
        }
        
        UpdatePositionAndRotation();
    }

    void PlayerAimMeshController::Update()
    {
        // 참조가 없으면 다시 시도
        if (!m_playerObject || !m_aimPointerMeshObject)
        {
            CacheReferences();
            if (!m_playerObject || !m_aimPointerMeshObject)
                return;
        }

        UpdateShootingState();
        UpdatePositionAndRotation();
        UpdateAnimation();
    }

    void PlayerAimMeshController::CacheReferences()
    {
        auto* scene = engine::SceneManager::Get().GetScene();
        if (!scene)
            return;

        // 플레이어 오브젝트
        if (!m_playerObject)
        {
            m_playerObject = scene->FindGameObject(m_playerObjectName);
        }

        // AimPointerMesh 오브젝트
        if (!m_aimPointerMeshObject)
        {
            m_aimPointerMeshObject = scene->FindGameObject(m_aimPointerMeshObjectName);
        }

        // AnimFSM (같은 GameObject에서)
        if (!m_animFSM)
        {
            m_animFSM = GetGameObject()->GetComponent<engine::AnimFSM>();
        }

        // Player의 LogicFSM
        if (!m_logicFSM && m_playerObject)
        {
            m_logicFSM = m_playerObject->GetComponent<engine::LogicFSM>();
        }

        // Player의 AimPointer
        if (!m_aimPointer && m_playerObject)
        {
            m_aimPointer = m_playerObject->GetComponent<AimPointer>();
        }
        
        // PlayerControllerScript 참조 및 콜백 등록
        if (!m_playerControllerScript && m_playerObject)
        {
            m_playerControllerScript = m_playerObject->GetComponent<PlayerControllerScript>();
            if (m_playerControllerScript)
            {
                m_playerControllerScript->RegisterFireCallback(this, [this]() { OnPlayerFired(); });
            }
        }
    }

    void PlayerAimMeshController::UpdatePositionAndRotation()
    {
        if (!m_playerObject || !m_aimPointerMeshObject)
            return;

        // ─────────────────────────────────────────────
        // 1. 위치: 플레이어 Transform의 XZ + 고정 Y
        // ─────────────────────────────────────────────
        engine::Vector3 playerPos = m_playerObject->GetTransform()->GetWorldPosition();
        engine::Vector3 newPosition(playerPos.x, m_fixedY, playerPos.z);
        GetTransform()->SetLocalPosition(newPosition);

        // ─────────────────────────────────────────────
        // 2. AimPointerMesh 위치 획득
        // ─────────────────────────────────────────────
        engine::Vector3 aimMeshPos = m_aimPointerMeshObject->GetTransform()->GetWorldPosition();

        // ─────────────────────────────────────────────
        // 3. 방향 벡터 계산 (XZ 평면)
        // ─────────────────────────────────────────────
        engine::Vector3 direction = aimMeshPos - newPosition;
        direction.y = 0.0f;

        float length = direction.Length();
        if (length < 0.0001f)
        {
            direction = engine::Vector3(0.0f, 0.0f, 1.0f);
        }
        else
        {
            direction.Normalize();
        }

        // ─────────────────────────────────────────────
        // 4. Forward/Backward 판단
        //    이동 방향(WASD) vs 에임 방향의 내적으로 판정
        //    (애니메이션 선택용, 회전과는 무관)
        // ─────────────────────────────────────────────
        UpdateForwardBackward(direction);

        // ─────────────────────────────────────────────
        // 5. 회전 계산: +Z가 항상 AimPointerMesh를 향하도록
        //    Backward 여부와 상관없이 항상 에임 방향을 바라봄
        //    → 캐릭터 정면이 항상 에임을 향함
        //    → Backward 애니메이션 시 "뒤로 걷는" 모션이 자연스럽게 표현됨
        // ─────────────────────────────────────────────
        float yawAngle = std::atan2(direction.x, direction.z);
        engine::Quaternion rotation = engine::Quaternion::CreateFromYawPitchRoll(yawAngle, 0.0f, 0.0f);
        GetTransform()->SetLocalRotation(rotation);
    }

    engine::Vector3 PlayerAimMeshController::GetMoveInputDirection() const
    {
        // ─────────────────────────────────────────────
        // WASD 입력 → 월드 좌표계 방향 벡터
        // W = +Z, S = -Z, A = -X, D = +X
        // ─────────────────────────────────────────────
        engine::Vector3 moveDir = engine::Vector3::Zero;

        if (engine::Input::IsKeyHeld(engine::Keys::W)) moveDir.z += 1.0f;
        if (engine::Input::IsKeyHeld(engine::Keys::S)) moveDir.z -= 1.0f;
        if (engine::Input::IsKeyHeld(engine::Keys::A)) moveDir.x -= 1.0f;
        if (engine::Input::IsKeyHeld(engine::Keys::D)) moveDir.x += 1.0f;       

        // 정규화 (대각선 이동 시 속도 보정)
        float length = moveDir.Length();
        if (length > 0.0001f)
        {
            moveDir /= length;
        }

        return moveDir;
    }

    void PlayerAimMeshController::UpdateForwardBackward(const engine::Vector3& aimDir)
    {
        // ─────────────────────────────────────────────
        // 이동 방향 결정
        // - 방향키 입력 있음: 입력 방향 = 앞
        // - 방향키 입력 없음: 에임 방향 = 앞 (항상 Forward)
        // ─────────────────────────────────────────────
        engine::Vector3 moveDir = GetMoveInputDirection();

        // 입력이 없으면 에임 방향을 이동 방향으로 사용 (항상 Forward)
        if (moveDir.LengthSquared() < 0.0001f)
        {
            m_isBackward = false;
            return;
        }

        // ─────────────────────────────────────────────
        // 이동 방향과 에임 방향의 내적으로 Forward/Backward 판정
        // 내적 > 0: 90도 이내 (Forward)
        // 내적 <= 0: 90도 이상 (Backward)
        // 히스테리시스 적용하여 떨림 방지
        // ─────────────────────────────────────────────
        float dot = moveDir.Dot(aimDir);

        if (m_isBackward)
        {
            // 현재 Backward → Forward 전환 조건
            if (dot > m_forwardThreshold)
            {
                m_isBackward = false;
            }
        }
        else
        {
            // 현재 Forward → Backward 전환 조건
            if (dot < m_backwardThreshold)
            {
                m_isBackward = true;
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 애니메이션 관련
    // ═══════════════════════════════════════════════════════════════
    void PlayerAimMeshController::InitializeAnimFSM()
    {
        if (!m_animFSM) return;

        m_animFSM->ClearStates();

        // ─────────────────────────────────────────────
        // 상/하체 분리 애니메이션 상태 등록
        // 비발사 상태: 상체 웨이트 0 (비활성화)
        // 발사 상태: 상체 웨이트 1 + Fire 애니메이션
        // ─────────────────────────────────────────────

        // 비발사 상태 (상체 레이어 비활성화)
        m_animFSM->AddSplitState("Idle", m_animName_Idle, true, "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("WalkForward", m_animName_WalkForward, true, "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("WalkBackward", m_animName_WalkBackward, true, "", false, 0.0f, 0.1f);

        // 발사 상태 (하체: 이동 애니, 상체: Fire 애니)
        m_animFSM->AddSplitState("IdleShoot", m_animName_Idle, true, m_animName_Fire, false, 1.0f, 0.1f);
        m_animFSM->AddSplitState("WalkForwardShoot", m_animName_WalkForward, true, m_animName_Fire, false, 1.0f, 0.1f);
        m_animFSM->AddSplitState("WalkBackwardShoot", m_animName_WalkBackward, true, m_animName_Fire, false, 1.0f, 0.1f);
    }

    void PlayerAimMeshController::UpdateAnimation()
    {
        if (!m_animFSM || !m_logicFSM) return;

        // ─────────────────────────────────────────────
        // LogicFSM 상태 + 이동 방향 → AnimFSM 상태 결정
        // ─────────────────────────────────────────────
        bool isMoving = m_logicFSM->GetBoolParameter("IsMoving");
        bool isShooting = m_isShooting;  // PCS 콜백으로 제어
        bool isBackward = m_isBackward;

        // AnimFSM 상태 결정
        std::string animState;

        if (isShooting)
        {
            if (!isMoving)
            {
                animState = "IdleShoot";
            }
            else if (isBackward)
            {
                animState = "WalkBackwardShoot";
            }
            else
            {
                animState = "WalkForwardShoot";
            }
        }
        else
        {
            if (!isMoving)
            {
                animState = "Idle";
            }
            else if (isBackward)
            {
                animState = "WalkBackward";
            }
            else
            {
                animState = "WalkForward";
            }
        }

        // AnimFSM에 상태 전달
        m_animFSM->SetAnimState(animState);
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void PlayerAimMeshController::OnGui()
    {
        ImGui::Text("=== PlayerAimMesh Controller ===");
        ImGui::Separator();

        // 참조 설정
        ImGui::Text("References:");
        ImGui::InputText("Player Object", &m_playerObjectName);
        ImGui::Text("Player: %s", m_playerObject ? "[OK]" : "[NOT FOUND]");

        ImGui::InputText("AimPointerMesh Object", &m_aimPointerMeshObjectName);
        ImGui::Text("AimPointerMesh: %s", m_aimPointerMeshObject ? "[OK]" : "[NOT FOUND]");

        ImGui::Separator();

        // 컴포넌트 상태
        ImGui::Text("Components:");
        ImGui::Text("AnimFSM: %s", m_animFSM ? "[OK]" : "[NOT FOUND]");
        ImGui::Text("LogicFSM (Player): %s", m_logicFSM ? "[OK]" : "[NOT FOUND]");
        ImGui::Text("AimPointer (Player): %s", m_aimPointer ? "[OK]" : "[NOT FOUND]");

        ImGui::Separator();

        // 위치 설정
        ImGui::Text("Position Settings:");
        ImGui::DragFloat("Fixed Y", &m_fixedY, 0.1f, -100.0f, 100.0f);

        ImGui::Separator();

        // 애니메이션 설정
        ImGui::Text("Animation Names:");
        ImGui::InputText("Idle", &m_animName_Idle);
        ImGui::InputText("WalkForward", &m_animName_WalkForward);
        ImGui::InputText("WalkBackward", &m_animName_WalkBackward);
        ImGui::InputText("Fire", &m_animName_Fire);

        ImGui::Separator();

        // Forward/Backward 판정 설정
        ImGui::Text("Forward/Backward Detection:");
        ImGui::DragFloat("Backward Threshold", &m_backwardThreshold, 0.01f, -1.0f, 0.0f);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Forward -> Backward transition (dot product threshold)\n-0.1 = about 96 degrees");
        }
        ImGui::DragFloat("Forward Threshold", &m_forwardThreshold, 0.01f, 0.0f, 1.0f);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Backward -> Forward transition (dot product threshold)\n0.1 = about 84 degrees");
        }

        ImGui::Separator();

        // Shooting 설정
        ImGui::Text("Shooting Settings:");
        ImGui::DragFloat("Shooting Duration", &m_shootingDuration, 0.01f, 0.0f, 2.0f);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("How long the shooting animation state is maintained after firing");
        }

        ImGui::Separator();

        // 런타임 상태
        ImGui::Text("Runtime State:");
        engine::Vector3 pos = GetTransform()->GetWorldPosition();
        engine::Vector3 forward = GetTransform()->GetForward();
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        ImGui::Text("Forward:  (%.2f, %.2f, %.2f)", forward.x, forward.y, forward.z);
        
        // 이동 방향 디버그
        engine::Vector3 moveDir = GetMoveInputDirection();
        ImGui::Text("Move Input: (%.2f, %.2f, %.2f)", moveDir.x, moveDir.y, moveDir.z);
        ImGui::Text("Is Backward: %s", m_isBackward ? "Yes" : "No");
        ImGui::Text("Is Shooting: %s (Timer: %.2f)", m_isShooting ? "Yes" : "No", m_shootingTimer);

        if (m_logicFSM)
        {
            ImGui::Text("LogicFSM State: %s", m_logicFSM->GetCurrentState().c_str());
        }
        
        ImGui::Text("PlayerController: %s", m_playerControllerScript ? "[OK]" : "[NOT FOUND]");
    }

    void PlayerAimMeshController::Save(engine::json& j) const
    {
        Object::Save(j);

        j["PlayerObjectName"] = m_playerObjectName;
        j["AimPointerMeshObjectName"] = m_aimPointerMeshObjectName;
        j["FixedY"] = m_fixedY;

        // 애니메이션 이름
        j["AnimName_Idle"] = m_animName_Idle;
        j["AnimName_WalkForward"] = m_animName_WalkForward;
        j["AnimName_WalkBackward"] = m_animName_WalkBackward;
        j["AnimName_Fire"] = m_animName_Fire;

        // Forward/Backward 판정 설정
        j["BackwardThreshold"] = m_backwardThreshold;
        j["ForwardThreshold"] = m_forwardThreshold;
        
        // Shooting 설정
        j["ShootingDuration"] = m_shootingDuration;
    }

    void PlayerAimMeshController::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "PlayerObjectName", m_playerObjectName);
        engine::JsonGet(j, "AimPointerMeshObjectName", m_aimPointerMeshObjectName);
        engine::JsonGet(j, "FixedY", m_fixedY);

        // 애니메이션 이름
        engine::JsonGet(j, "AnimName_Idle", m_animName_Idle);
        engine::JsonGet(j, "AnimName_WalkForward", m_animName_WalkForward);
        engine::JsonGet(j, "AnimName_WalkBackward", m_animName_WalkBackward);
        engine::JsonGet(j, "AnimName_Fire", m_animName_Fire);

        // Forward/Backward 판정 설정
        engine::JsonGet(j, "BackwardThreshold", m_backwardThreshold);
        engine::JsonGet(j, "ForwardThreshold", m_forwardThreshold);
        
        // Shooting 설정
        engine::JsonGet(j, "ShootingDuration", m_shootingDuration);
    }

    // ═══════════════════════════════════════════════════════════════
    // Shooting 상태 관리
    // ═══════════════════════════════════════════════════════════════
    void PlayerAimMeshController::OnPlayerFired()
    {
        // 발사할 때마다 타이머 리셋 (연속 발사 중에는 계속 true)
        m_isShooting = true;
        m_shootingTimer = m_shootingDuration;
    }

    void PlayerAimMeshController::UpdateShootingState()
    {
        if (m_isShooting)
        {
            m_shootingTimer -= engine::Time::DeltaTime();
            if (m_shootingTimer <= 0.0f)
            {
                m_isShooting = false;
                m_shootingTimer = 0.0f;
            }
        }
    }
}
