#include "GamePCH.h"
#include "PlayerAimMeshController.h"

#include "Script/AimPointer.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/AnimFSM.h>
#include <Framework/Object/Component/LogicFSM.h>
#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Engine/Core/System/Input.h>
#include <Engine/Core/System/MyTime.h>
#include <Engine/Framework/Object/Component/Renderer/AfterimageRenderer.h>
#include <Engine/Common/Math/MathUtility.h>

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
            m_animFSM->SetScriptControlled(true);  // LogicFSM 콜백으로 Idle 재생되지 않게 (한 번 클릭 시 뚜둑 방지)
            m_animFSMInitialized = true;
        }

        // 상체 레이어 마스크: Walk+발사 시 Spine 이상만 상체 애니(Fire) 적용
        auto* animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
        if (animator && m_animFSM)
        {
            int upperLayer = m_animFSM->GetUpperBodyLayerIndex();
            if (animator->GetLayerCount() <= upperLayer)
            {
                animator->AddLayer("Upper Body", 0.0f);
            }
            std::vector<std::string> spineBones = { m_animFSM->GetSpineBoneName() };
            animator->SetLayerMask(upperLayer, spineBones, true, true);

            // 총 본이 손 본을 따르도록 (리그에서 총이 루트 직계일 때 코드로 보정)
            if (!m_gunBoneName.empty() && !m_handBoneName.empty())
            {
                animator->SetBoneFollowBone(m_gunBoneName, m_handBoneName);
            }
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
        
        // ─────────────────────────────────────────────
        // Fire 애니메이션 재생 속도 동기화 및 발사 프레임 확인
        // ─────────────────────────────────────────────
        if (m_animFSM && m_playerControllerScript && m_logicFSM)
        {
            std::string currentState = m_logicFSM->GetCurrentState();
            bool isShooting = (currentState == "IdleShoot" || currentState == "WalkShoot");
            
            if (isShooting)
            {
                // 발사 간격(fireRate)을 직접 사용하여 애니메이션 속도 계산
                float fireRate = m_playerControllerScript->GetFireRate();
                const float baseFireRate = 0.7f;  // 기본 발사 간격 (초)
                float animSpeed = (fireRate > 0.001f) ? (baseFireRate / fireRate) : 1.0f;
                
                // AnimFSM의 SkeletalAnimator 가져오기
                auto* animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
                if (animator)
                {
                    int targetLayer = -1;
                    if (currentState == "IdleShoot")
                    {
                        targetLayer = m_animFSM->GetBaseLayerIndex();
                    }
                    else if (currentState == "WalkShoot")
                    {
                        targetLayer = m_animFSM->GetUpperBodyLayerIndex();
                    }
                    
                    if (targetLayer >= 0)
                    {
                        std::string currentAnim = animator->GetCurrentAnimationName(targetLayer);
                        if (currentAnim == m_animName_Fire)
                        {
                            // 애니메이션 속도 업데이트 (발사 속도에 맞춤)
                            animator->SetLayerSpeed(targetLayer, animSpeed);
                            // 발사 프레임 통과 시 한 번만 SetCanFireNow(true) (총알↔모션 동기화)
                            float normalizedTime = animator->GetNormalizedTime(targetLayer);
                            if (normalizedTime >= m_fireAnimShootFrameTime && m_prevFireNormalizedTime < m_fireAnimShootFrameTime)
                                m_playerControllerScript->SetCanFireNow(true);
                            m_prevFireNormalizedTime = normalizedTime;
                        }
                        else
                        {
                            m_prevFireNormalizedTime = -1.0f;
                        }
                    }
                    else
                    {
                        m_prevFireNormalizedTime = -1.0f;
                    }
                }
            }
            else
            {
                m_prevFireNormalizedTime = -1.0f;
                m_playerControllerScript->SetCanFireNow(false);
            }
        }
        
        // 메시 쪽 AnimFSM은 플레이어(로직) 오브젝트가 아니라 이 오브젝트에 붙어 있으므로 여기서 UpdateFSM 호출 (상체 weight 보간, procedural)
        if (m_animFSM)
            m_animFSM->UpdateFSM();
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
        // 5. 회전
        //    - Idle/가만히 있을 때: 항상 에임 방향으로 부드럽게 회전 (보간).
        //    - 이동 중(Walk): 임계각 초과일 때만 하체를 에임 쪽으로 보간 회전; 그 미만이면 상체만 에임 추적.
        // ─────────────────────────────────────────────
        float targetYaw = std::atan2(direction.x, direction.z);
        engine::Quaternion targetRotation = engine::Quaternion::CreateFromYawPitchRoll(targetYaw, 0.0f, 0.0f);
        engine::Quaternion currentRotation = GetTransform()->GetLocalRotation();

        bool shouldRotateTowardAim = false;
        if (m_logicFSM && m_logicFSM->GetBoolParameter("IsMoving"))
        {
            // 이동 중: 걸으며 쏠 때는 임계값 0(항상 에임 추적), 그냥 걸을 때만 설정된 임계값 사용 (옆/뒤 달리기용)
            bool isShooting = m_logicFSM->GetBoolParameter("IsShooting");
            float effectiveThreshold = isShooting ? 0.0f : m_lowerBodyAimThresholdDeg;
            engine::Vector3 currentForward = GetTransform()->GetForward();
            currentForward.y = 0.0f;
            if (currentForward.LengthSquared() >= 0.0001f)
            {
                currentForward.Normalize();
                float dot = currentForward.Dot(direction);
                engine::Vector3 cross = currentForward.Cross(direction);
                float angleDeg = engine::ToDegree(std::atan2(cross.y, dot));
                if (angleDeg < 0.0f) angleDeg = -angleDeg;
                shouldRotateTowardAim = (angleDeg > effectiveThreshold);
            }
            else
                shouldRotateTowardAim = true;
        }
        else
            shouldRotateTowardAim = true;  // Idle/가만히: 항상 에임 쪽으로

        if (shouldRotateTowardAim)
        {
            float t = std::min(1.0f, m_lowerBodyTurnSpeed * engine::Time::DeltaTime());
            engine::Quaternion newRotation = engine::Quaternion::Slerp(currentRotation, targetRotation, t);
            GetTransform()->SetLocalRotation(newRotation);
        }
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
        // 비발사: 상체 레이어 비활성화 (하체 애니만 전신)
        // Idle+발사: 전신 Fire (상하체 분리 없음)
        // Walk+발사: 하체 Walk + 상체 Fire (Split, 추후 procedural 보정)
        // ─────────────────────────────────────────────

        m_animFSM->AddSplitState("Idle", m_animName_Idle, true, "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("WalkForward", m_animName_WalkForward, true, "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("WalkBackward", m_animName_WalkBackward, true, "", false, 0.0f, 0.1f);

        // Idle+발사: 전신 Fire 단일 재생 (Default). 꾹 누르는 동안 루프
        m_animFSM->AddDefaultState("IdleShoot", m_animName_Fire, true, 0.1f, 0, 1.0f);

        // Walk+발사: 하체 이동 애니 + 상체 Fire (Split). 상체 Fire는 꾹 누르는 동안 계속 재생(루프)
        m_animFSM->AddSplitState("WalkForwardShoot", m_animName_WalkForward, true, m_animName_Fire, true, 1.0f, 0.1f);
        m_animFSM->AddSplitState("WalkBackwardShoot", m_animName_WalkBackward, true, m_animName_Fire, true, 1.0f, 0.1f);
    }

    void PlayerAimMeshController::UpdateAnimation()
    {
        if (!m_animFSM || !m_logicFSM) return;

        // ─────────────────────────────────────────────
        // LogicFSM 상태 + 이동 방향 → AnimFSM 상태 결정
        // ─────────────────────────────────────────────
        bool isMoving = m_logicFSM->GetBoolParameter("IsMoving");
        // 발사 애니: 마우스 홀드/클릭 순간이거나, 방금 발사 후 m_shootingDuration 동안 Shoot 유지 (한 번 클릭 시 뚜둑 없이 바로 Fire 재생)
        bool mouseHeldOrPressed = m_logicFSM->GetBoolParameter("IsShooting") || engine::Input::IsMousePressed(engine::Input::Buttons::LEFT);
        bool isShooting = mouseHeldOrPressed || m_isShooting;
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

        // Procedural 상체 회전: Walk+발사일 때만 활성화 (Idle+발사는 전신 Fire라 보정 불필요)
        bool needProceduralAim = (animState == "WalkForwardShoot" || animState == "WalkBackwardShoot");
        m_animFSM->SetProceduralAimEnabled(needProceduralAim);

        if (needProceduralAim)
        {
            // 에임 방향 대비 상체 Yaw (캐릭터 전방 → 에임 방향 각도). 오프셋 + 스케일(옆 조준 시 손 방향 보정)
            float aimYaw = (CalculateAimYaw() + m_upperBodyAimOffsetDeg) * m_upperBodyYawScale;
            m_animFSM->SetUpperBodyYaw(aimYaw);

            // 하체(메시) 회전 보정: 하체가 회전한 만큼 상체 Yaw에서 빼서 월드 기준 에임 유지
            float currentLowerYaw = GetLowerBodyYawDegrees();
            float deltaYaw = currentLowerYaw - m_prevLowerBodyYawDeg;
            while (deltaYaw > 180.0f)  deltaYaw -= 360.0f;
            while (deltaYaw < -180.0f) deltaYaw += 360.0f;
            m_animFSM->OffsetCurrentYaw(-deltaYaw);
            m_prevLowerBodyYawDeg = currentLowerYaw;
        }
        else
        {
            m_prevLowerBodyYawDeg = GetLowerBodyYawDegrees();
        }
    }

    float PlayerAimMeshController::CalculateAimYaw() const
    {
        if (!m_aimPointerMeshObject) return 0.0f;

        engine::Vector3 meshPos = GetTransform()->GetWorldPosition();
        engine::Vector3 aimPos = m_aimPointerMeshObject->GetTransform()->GetWorldPosition();
        engine::Vector3 toAim = aimPos - meshPos;
        toAim.y = 0.0f;
        if (toAim.LengthSquared() < 0.001f) return 0.0f;
        toAim.Normalize();

        engine::Vector3 forward = GetTransform()->GetForward();
        forward.y = 0.0f;
        if (forward.LengthSquared() < 0.001f) return 0.0f;
        forward.Normalize();

        float dotProduct = forward.Dot(toAim);
        engine::Vector3 crossProduct = forward.Cross(toAim);
        float angleRad = std::atan2(crossProduct.y, dotProduct);
        return engine::ToDegree(angleRad);
    }

    float PlayerAimMeshController::GetLowerBodyYawDegrees() const
    {
        engine::Vector3 forward = GetTransform()->GetForward();
        forward.y = 0.0f;
        if (forward.LengthSquared() < 0.001f) return 0.0f;
        forward.Normalize();
        float rad = std::atan2(forward.x, forward.z);
        return engine::ToDegree(rad);
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
        ImGui::DragFloat("Lower Body Aim Threshold (deg)", &m_lowerBodyAimThresholdDeg, 1.0f, 0.0f, 180.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("이동 중일 때만 적용. 에임이 하체와 이 각도 이상 벌어졌을 때만 하체 회전");
        ImGui::DragFloat("Lower Body Turn Speed", &m_lowerBodyTurnSpeed, 0.5f, 0.5f, 30.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("하체 회전 보간 속도. 클수록 빨리 에임 방향으로 맞춤");

        ImGui::Separator();

        // 애니메이션 설정
        ImGui::Text("Animation Names:");
        ImGui::InputText("Idle", &m_animName_Idle);
        ImGui::InputText("WalkForward", &m_animName_WalkForward);
        ImGui::InputText("WalkBackward", &m_animName_WalkBackward);
        ImGui::InputText("Fire", &m_animName_Fire);
        ImGui::DragFloat("Fire Anim Shoot Frame Time", &m_fireAnimShootFrameTime, 0.01f, 0.0f, 1.0f);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Fire 애니메이션에서 실제 총알 발사 모션이 나오는 시간 (정규화된 시간, 0.0~1.0). 예: 0.2 = 애니메이션의 20% 지점");
        }
        ImGui::DragFloat("Upper Body Aim Offset (deg)", &m_upperBodyAimOffsetDeg, 1.0f, -90.0f, 90.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Walk+발사 시 상체 조준 Yaw 보정. Fire 애니가 비스듬히 서서 쏘면 조정 (왼쪽 보면 +)");
        ImGui::DragFloat("Upper Body Yaw Scale", &m_upperBodyYawScale, 0.05f, 0.5f, 2.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("옆 조준 시 손 방향 어긋남 보정. 1=기본, 손이 덜 돌면 >1, 과하게 돌면 <1");

        ImGui::Separator();

        // 총 본 → 손 본 연동 (리그에서 총이 루트 직계일 때)
        ImGui::Text("Gun Bone Follow Hand:");
        ImGui::InputText("Gun Bone Name", &m_gunBoneName);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("스켈레톤의 총 메쉬 본 이름. 비우면 미사용");
        ImGui::InputText("Hand Bone Name", &m_handBoneName);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("총이 따라갈 손 본 이름. 둘 다 설정 시에만 연동");

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
        j["LowerBodyAimThresholdDeg"] = m_lowerBodyAimThresholdDeg;
        j["LowerBodyTurnSpeed"] = m_lowerBodyTurnSpeed;

        // 애니메이션 이름
        j["AnimName_Idle"] = m_animName_Idle;
        j["AnimName_WalkForward"] = m_animName_WalkForward;
        j["AnimName_WalkBackward"] = m_animName_WalkBackward;
        j["AnimName_Fire"] = m_animName_Fire;
        j["FireAnimShootFrameTime"] = m_fireAnimShootFrameTime;
        j["UpperBodyAimOffsetDeg"] = m_upperBodyAimOffsetDeg;
        j["UpperBodyYawScale"] = m_upperBodyYawScale;
        j["GunBoneName"] = m_gunBoneName;
        j["HandBoneName"] = m_handBoneName;

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
        engine::JsonGet(j, "LowerBodyAimThresholdDeg", m_lowerBodyAimThresholdDeg);
        engine::JsonGet(j, "LowerBodyTurnSpeed", m_lowerBodyTurnSpeed);

        // 애니메이션 이름
        engine::JsonGet(j, "AnimName_Idle", m_animName_Idle);
        engine::JsonGet(j, "AnimName_WalkForward", m_animName_WalkForward);
        engine::JsonGet(j, "AnimName_WalkBackward", m_animName_WalkBackward);
        engine::JsonGet(j, "AnimName_Fire", m_animName_Fire);
        engine::JsonGet(j, "FireAnimShootFrameTime", m_fireAnimShootFrameTime);
        engine::JsonGet(j, "UpperBodyAimOffsetDeg", m_upperBodyAimOffsetDeg);
        engine::JsonGet(j, "UpperBodyYawScale", m_upperBodyYawScale);
        engine::JsonGet(j, "GunBoneName", m_gunBoneName);
        engine::JsonGet(j, "HandBoneName", m_handBoneName);

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
        // Fire 애니는 AnimFSM 전환으로만 재생 (Play() 호출 안 함 → Idle→Fire 블렌딩 유지)
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
