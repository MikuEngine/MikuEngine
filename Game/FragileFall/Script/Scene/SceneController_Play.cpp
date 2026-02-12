#include "GamePCH.h"
#include "SceneController_Play.h"

#include <Core/App/AppContext.h>
#include <Core/App/WinApp.h>
#include <Core/System/MyTime.h>

#include <Framework/System/SoundSystem.h>
#include <Framework/Asset/Prefab.h>

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Object/Component/UI/UIButton.h>
#include <Framework/Object/Component/UI/UIText.h>
#include <Framework/Object/Component/UI/UIProgressBar.h>
#include <Framework/Object/Component/UI/UIImage.h>

#include "Manager/TimeScaler.h"
#include "Manager/StageManager.h"
#include <Manager/LoadingScreenDrawer.h>
#include <Script/AimModeController.h>
#include <Script/CharacterScript/Player/PlayerControllerScript.h>
#include <Manager/MessageCatalog.h>

#include <Script/UI/UIPopUpAnimator.h>
#include <Script/UI/UIMessageQueue.h>

#include "Scene/GameScene.h"
#include <Script/CameraEffectScript.h>
#include <Script/CrystalIceFillControllerScript.h>
#include <Script/CharacterScript/Player/PlayerAimMeshController.h>
#include <Framework/Object/Component/Animator/SkeletalAnimator.h>

namespace game
{
    namespace
    {
        // 마우스 감도 예외처리
        constexpr float kSensMin = 0.2f;
        constexpr float kSensMax = 3.0f;

        static float Clamp(float v, float a, float b)
        {
            if (v < a) return a;
            if (v > b) return b;
            return v;
        }

        static float SliderToSensitivity(float t01)
        {
            t01 = Clamp(t01, 0.f, 1.f);
            return kSensMin + t01 * (kSensMax - kSensMin);
        }

        // sensitivity -> slider(0~1)
        static float SensitivityToSlider(float sens)
        {
            sens = Clamp(sens, kSensMin, kSensMax);
            return (sens - kSensMin) / (kSensMax - kSensMin);
        }

        static game::MessageCatalog g_msg;

        static std::string GetDeathCrystalColorSuffix(const std::string& mapPrefabName)
        {
            std::string color = StageManager::GetMapColorFromPrefabName(mapPrefabName);
            // 플레이어 사망 크리스탈 프리팹은 White 대신 Gray를 사용
            if (color == "White")
                color = "Gray";
            return color;
        }

        static std::string GetDeathBreakEffectPrefab(const std::string& colorSuffix)
        {
            if (colorSuffix == "Gray")   return "Effect_Break_V1.01_big_Gray";
            if (colorSuffix == "Blue")   return "Effect_Break_V1.01_big_Blue";
            if (colorSuffix == "Red")    return "Effect_Break_V1.01_big_Red";
            if (colorSuffix == "Green")  return "Effect_Break_V1.01_big_Green";
            if (colorSuffix == "Purple") return "Effect_Break_V1.01_big_Purple";
            return "Effect_Break_V1.01_big_white";
        }

        static CrystalIceFillControllerScript* FindCrystalFillInHierarchy(engine::Transform* root)
        {
            if (!root) return nullptr;

            engine::GameObject* owner = root->GetGameObject();
            if (owner)
            {
                if (auto* fill = owner->GetComponent<CrystalIceFillControllerScript>())
                    return fill;
            }

            for (engine::Transform* child : root->GetChildren())
            {
                if (auto* fill = FindCrystalFillInHierarchy(child))
                    return fill;
            }
            return nullptr;
        }
    }

    void SceneController_Play::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        StageManager::Get().BeginStage(); // 아직 맵 프리팹이 없어서 스테이지 세팅 불가

        g_msg.Load("Resource/Data/Message/MessageTable.csv");

        if (auto* go = engine::GameObject::Find("UIMessageQueue"))
            if (auto* q = go->GetComponent<game::UIMessageQueue>())
                q->SetCatalog(&g_msg);

        // Buttons
        BindButton("UI_OpenMenu", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->OpenMenu(); });
        BindButton("UI_OpenOption", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->OpenOption(); });
        BindButton("UI_BackToPlay", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->BackToPlay(); });
        BindButton("UI_BackToMain", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->CheckBackToMain(true); });
        BindButton("UI_CloseButton_Option", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->Back(); });

        BindButton("OK_Button", [self = engine::Ptr<SceneController_Play>(this)]() {if (self) self->BackToLobby(); });
        BindButton("Cancel_Button", [self = engine::Ptr<SceneController_Play>(this)] {if (self) self->CheckBackToMain(false); });

        BindButton("UI_OpenOption", [self = engine::Ptr<SceneController_Play>(this)](bool hovered) {});
        BindButton("UI_BackToPlay", [self = engine::Ptr<SceneController_Play>(this)](bool hovered) {});
        BindButton("UI_BackToMain", [self = engine::Ptr<SceneController_Play>(this)](bool hovered) {});
        BindButton("UI_CloseButton_Option", [self = engine::Ptr<SceneController_Play>(this)](bool hovered) {});
        BindButton("OK_Button", [self = engine::Ptr<SceneController_Play>(this)](bool hovered) {});
        BindButton("Cancel_Button", [self = engine::Ptr<SceneController_Play>(this)](bool hovered) {});

        // Sliders
        BindSlider("UI_BGMSlider", [self = engine::Ptr<SceneController_Play>(this)](float v) {if (self) self->OnBGMChanged(v); });
        BindSlider("UI_SFXSlider", [self = engine::Ptr<SceneController_Play>(this)](float v) {if (self) self->OnSFXChanged(v); });
        BindSlider("UI_SensitivitySlider", [self = engine::Ptr<SceneController_Play>(this)](float v) {if (self) self->SetSensitivity(v); });
    }

    void SceneController_Play::Start()
    {
        if (!TimeScaler::IsActive())
            TimeScaler::PlayWorld();

        auto* go = engine::GameObject::Find("Player");
        if (go)
        {
            m_aimMode = go->GetComponent<AimModeController>();
            m_playerScript = go->GetComponent<PlayerControllerScript>();

            // 플레이어 애니메이션 메쉬 오브젝트에서 SkeletalAnimator 검색
            auto* animMeshGO = engine::GameObject::Find("PlayerAnimMesh");
            if (animMeshGO)
                m_playerAnimator = animMeshGO->GetComponent<engine::SkeletalAnimator>();
        }

        // 카메라 참조 캐시
        m_mainCamera = engine::GameObject::Find("MainCamera");
        if (m_mainCamera)
        {
            auto* camGO = m_mainCamera;
            m_cameraEffect = camGO->GetComponent<CameraEffectScript>();
        }

        m_menuPopUp = engine::GameObject::Find("Panel_Menu");
        if (m_menuPopUp) m_menuPopUp->SetActive(false);

        m_optionPopUp = engine::GameObject::Find("UI_OptionPopUp");
        if (m_optionPopUp) m_optionPopUp->SetActive(false);

        m_blocker = engine::GameObject::Find("Panel_Blocker");
        if (m_blocker) m_blocker->SetActive(false);

        m_realGiveupPopUp = engine::GameObject::Find("UI_RealGiveupPopUp");
        if (m_realGiveupPopUp) m_realGiveupPopUp->SetActive(false);

        m_failPanel = engine::GameObject::Find("Panel_Fail");
        if (m_failPanel) m_failPanel->SetActive(false);
        m_deathCrystalInstance = nullptr;

        m_isMenuOpen = false;
        m_isOptionOpen = false;
        m_isDead = false;
        m_failElapsedSec = 0.0f;
        m_failReturnTriggered = false;
        m_deathCinematicActive = false;
        m_deathCinematicElapsed = 0.0f;
        m_deathCinematicDone = false;
        m_deathCameraApproachRatio = Clamp(m_deathCameraApproachRatio, 0.0f, 1.0f);
        m_deathCameraTargetYOffset = Clamp(m_deathCameraTargetYOffset, -10.0f, 10.0f);
        m_deathCameraMoveSpeed = Clamp(m_deathCameraMoveSpeed, 0.1f, 5.0f);
        m_deathCrystalBurstTriggered = false;
        m_deathCrystalColorSuffix = "Gray";

        if (!m_isDead) TimeScaler::PlayWorld();

        auto& app = engine::AppContext::GetApp();
        const auto& s = app.GetUserSettings();

        if (auto* go = engine::GameObject::Find("UI_BGMSlider"))
            if (auto* slider = go->GetComponent<engine::UISlider>())
                slider->SetValue(s.audio.bgm, false);

        if (auto* go = engine::GameObject::Find("UI_SFXSlider"))
            if (auto* slider = go->GetComponent<engine::UISlider>())
                slider->SetValue(s.audio.sfx, false);

        if (auto* go = engine::GameObject::Find("UI_SensitivitySlider"))
            if (auto* slider = go->GetComponent<engine::UISlider>())
                slider->SetValue(SensitivityToSlider(s.controls.mouseSensitivity), false);

        // bgm sound
        engine::SoundSystem::Get().PlayBGM("Combat_1");
    }

    void SceneController_Play::Update()
    {
        StageManager::Get().Update();

        // 플레이어 사망 시 실패 (HP 0, 프레자일 100%, 또는 Dead 상태)
        if (!m_isDead && m_playerScript)
        {
            bool fragileFull = m_playerScript->GetFragileGaugeCurrent() >= m_playerScript->GetFragileGaugeMax();
            bool hpZero = m_playerScript->GetCurrentHp() <= 0.0f;
            bool playerDeadState = (m_playerScript->GetCurrentState() == "Dead");
            if (fragileFull || hpZero || playerDeadState)
                Fail();

            // sound
            if (fragileFull)
            {
                engine::SoundSystem::Get().Play("Player_Dead_Fragile", "SFX/Player");
            }
        }

        if (!m_isDead && engine::Input::IsKeyPressed(engine::Keys::Escape))
        {
            engine::SoundSystem::Get().PlayUI("UI_Click");

            if (m_isOptionOpen) { Back(); return; }
            if (m_isGiveupOpen) { CheckBackToMain(false); return; }
            if (m_isMenuOpen) { SetMenuOpen(false); return; }

            SetMenuOpen(true);
        }

        // ─── 사망 연출 업데이트 ───
        if (m_isDead && m_deathCinematicActive && !m_deathCinematicDone)
        {
            float dt = engine::Time::DeltaTime();
            m_deathCinematicElapsed += dt;

            float t = (m_deathCinematicDuration > 0.0f)
                ? std::min(m_deathCinematicElapsed / m_deathCinematicDuration, 1.0f)
                : 1.0f;

            // 카메라 워킹: 시작 위치 -> 플레이어 쪽 목표 위치로 직접 보간
            if (m_mainCamera && m_mainCamera->GetTransform() && m_playerScript && m_playerScript->GetTransform())
            {
                const engine::Vector3 playerPos = m_playerScript->GetTransform()->GetWorldPosition();
                engine::Vector3 camTarget = m_deathCameraStartPos;
                camTarget.x = m_deathCameraStartPos.x + (playerPos.x - m_deathCameraStartPos.x) * m_deathCameraApproachRatio;
                camTarget.z = m_deathCameraStartPos.z + (playerPos.z - m_deathCameraStartPos.z) * m_deathCameraApproachRatio;
                camTarget.y = playerPos.y + m_deathCameraTargetYOffset;

                // 기본 연출 시간 대비 더 빠르게 접근하도록 속도 배율 적용
                const float fastT = std::min(1.0f, t * m_deathCameraMoveSpeed);
                engine::Vector3 camNewPos = m_deathCameraStartPos + (camTarget - m_deathCameraStartPos) * fastT;
                auto* camTr = m_mainCamera->GetTransform();
                camTr->SetLocalPosition(camNewPos);

                // 카메라가 항상 (플레이어 + 오프셋) 지점을 바라보게 회전
                const engine::Vector3 lookTarget = playerPos + m_deathCameraLookAtOffset;
                engine::Vector3 toPlayer = lookTarget - camNewPos;
                if (toPlayer.LengthSquared() > 0.0001f)
                {
                    const float yaw = std::atan2(toPlayer.x, toPlayer.z);
                    const float horizontal = std::sqrt((toPlayer.x * toPlayer.x) + (toPlayer.z * toPlayer.z));
                    const float pitch = -std::atan2(toPlayer.y, horizontal);
                    const engine::Quaternion lookRot = engine::Quaternion::CreateFromYawPitchRoll(yaw, pitch, 0.0f);
                    camTr->SetLocalRotation(lookRot);
                }
            }

            // 연출 완료
            if (t >= 1.0f)
            {
                // 크리스탈이 가득 찬 시점에 터뜨린다.
                if (!m_deathCrystalBurstTriggered && m_deathCrystalInstance)
                {
                    if (auto* crystalTr = m_deathCrystalInstance->GetTransform())
                    {
                        auto effect = engine::Prefab::Instantiate(GetDeathBreakEffectPrefab(m_deathCrystalColorSuffix));
                        if (effect && effect->GetTransform())
                        {
                            effect->GetTransform()->SetWorldMatrix(crystalTr->GetWorld());
                            effect->GetTransform()->SetLocalScale(engine::Vector3(1.2f, 1.2f, 1.2f));
                        }
                    }
                    engine::SoundSystem::Get().Play("Player_Break_Random", "SFX/Player");
                    m_deathCrystalInstance->Destroy();
                    m_deathCrystalInstance = nullptr;

                    // 플레이어 자체 Destroy 대신 렌더용 메쉬만 비활성화
                    if (auto* playerAnimMesh = engine::GameObject::Find("PlayerAnimMesh"))
                        playerAnimMesh->SetActive(false);

                    m_deathCrystalBurstTriggered = true;
                }

                m_deathCinematicDone = true;
                m_deathCinematicActive = false;

                // FailPanel 표시
                if (m_failPanel)
                    m_failPanel->SetActive(true);
            }
        }

        // ─── FailPanel 표시 후 자동 로비 복귀 ───
        if (m_isDead && m_deathCinematicDone && m_autoReturnToLobbyOnFail && !m_failReturnTriggered)
        {
            m_failElapsedSec += engine::Time::DeltaTime();
            if (m_failElapsedSec >= m_failAutoReturnDelaySec)
            {
                m_failReturnTriggered = true;
                BackToLobby();
                return;
            }
        }

        // Debug
        if (engine::Input::IsKeyPressed(engine::Keys::F4))
        {
            Fail();
        }
        
        // Debug Cheat: F3 + B로 즉시 보스(10스테이지) 이동
        if (engine::Input::IsKeyHeld(engine::Keys::F3) && engine::Input::IsKeyPressed(engine::Keys::B))
        {
            StageManager::Get().RequestWarpToBossStageCheat();
            return;
        }

        if (engine::Input::IsKeyPressed(engine::Keys::K))
        {
            if (auto* go = engine::GameObject::Find("UIMessageQueue"))
                if (auto* q = go->GetComponent<game::UIMessageQueue>())
                    q->PushMessageKey("Kill_001");
        }

        if (engine::Input::IsKeyHeld(engine::Keys::F3) && engine::Input::IsKeyPressed(engine::Keys::P))
        {
            // 실제 플레이어의 TakeDamage를 호출하여 전체 시스템을 테스트합니다.
            if (m_playerScript)
            {
                // 데미지 1.0을 주면서 피격 콜백과 HP 감소가 정상 작동하는지 확인
                m_playerScript->TakeDamage(1.0f);

                //if (m_hitImage) {
                //     m_hitImage->SetEffect(engine::UIEffectType::ScreenHit);
                //     m_hitImage->SetEffectParam(0, { 1.0f, 0.0f, 0.0f, 0.0f });
                //}
            }
        }
    }

    void SceneController_Play::OnGui()
    {
        ImGui::Text("=== Death Settings ===");
        if (ImGui::DragFloat("Death Cinematic Duration (sec)", &m_deathCinematicDuration, 0.1f, 0.5f, 10.0f, "%.1f"))
        {
            m_deathCinematicDuration = Clamp(m_deathCinematicDuration, 0.5f, 10.0f);
        }
        if (ImGui::DragFloat("Death Camera Approach Ratio", &m_deathCameraApproachRatio, 0.01f, 0.0f, 1.0f, "%.2f"))
        {
            m_deathCameraApproachRatio = Clamp(m_deathCameraApproachRatio, 0.0f, 1.0f);
        }
        if (ImGui::DragFloat("Death Camera Y Offset", &m_deathCameraTargetYOffset, 0.05f, -10.0f, 10.0f, "%.2f"))
        {
            m_deathCameraTargetYOffset = Clamp(m_deathCameraTargetYOffset, -10.0f, 10.0f);
        }
        ImGui::DragFloat3("Death Camera LookAt Offset", &m_deathCameraLookAtOffset.x, 0.05f, -10.0f, 10.0f, "%.2f");
        if (ImGui::DragFloat("Death Camera Move Speed", &m_deathCameraMoveSpeed, 0.05f, 0.1f, 5.0f, "%.2f"))
        {
            m_deathCameraMoveSpeed = Clamp(m_deathCameraMoveSpeed, 0.1f, 5.0f);
        }
        ImGui::Checkbox("Auto Return Lobby On Fail", &m_autoReturnToLobbyOnFail);
        if (ImGui::DragFloat("Fail Auto Return Delay (sec)", &m_failAutoReturnDelaySec, 0.1f, 0.0f, 20.0f, "%.1f"))
        {
            m_failAutoReturnDelaySec = Clamp(m_failAutoReturnDelaySec, 0.0f, 20.0f);
        }

        // 런타임 상태 표시
        if (m_isDead)
        {
            ImGui::Separator();
            ImGui::Text("Dead: Cinematic %.1f / %.1f | FailTimer %.1f / %.1f",
                m_deathCinematicElapsed, m_deathCinematicDuration,
                m_failElapsedSec, m_failAutoReturnDelaySec);
        }
    }

    void SceneController_Play::Save(engine::json& j) const
    {
        Object::Save(j);
        j["AutoReturnToLobbyOnFail"] = m_autoReturnToLobbyOnFail;
        j["FailAutoReturnDelaySec"] = m_failAutoReturnDelaySec;
        j["DeathCinematicDuration"] = m_deathCinematicDuration;
        j["DeathCameraApproachRatio"] = m_deathCameraApproachRatio;
        j["DeathCameraTargetYOffset"] = m_deathCameraTargetYOffset;
        j["DeathCameraMoveSpeed"] = m_deathCameraMoveSpeed;
        j["DeathCameraLookAtOffset"] = { m_deathCameraLookAtOffset.x, m_deathCameraLookAtOffset.y, m_deathCameraLookAtOffset.z };
    }

    void SceneController_Play::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "AutoReturnToLobbyOnFail", m_autoReturnToLobbyOnFail);
        engine::JsonGet(j, "FailAutoReturnDelaySec", m_failAutoReturnDelaySec);
        engine::JsonGet(j, "DeathCinematicDuration", m_deathCinematicDuration);
        engine::JsonGet(j, "DeathCameraApproachRatio", m_deathCameraApproachRatio);
        engine::JsonGet(j, "DeathCameraTargetYOffset", m_deathCameraTargetYOffset);
        engine::JsonGet(j, "DeathCameraMoveSpeed", m_deathCameraMoveSpeed);
        if (j.contains("DeathCameraLookAtOffset") && j["DeathCameraLookAtOffset"].is_array() && j["DeathCameraLookAtOffset"].size() >= 3)
        {
            m_deathCameraLookAtOffset.x = j["DeathCameraLookAtOffset"][0].get<float>();
            m_deathCameraLookAtOffset.y = j["DeathCameraLookAtOffset"][1].get<float>();
            m_deathCameraLookAtOffset.z = j["DeathCameraLookAtOffset"][2].get<float>();
        }
        m_failAutoReturnDelaySec = Clamp(m_failAutoReturnDelaySec, 0.0f, 20.0f);
        m_deathCinematicDuration = Clamp(m_deathCinematicDuration, 0.5f, 10.0f);
        m_deathCameraApproachRatio = Clamp(m_deathCameraApproachRatio, 0.0f, 1.0f);
        m_deathCameraTargetYOffset = Clamp(m_deathCameraTargetYOffset, -10.0f, 10.0f);
        m_deathCameraMoveSpeed = Clamp(m_deathCameraMoveSpeed, 0.1f, 5.0f);
    }

    void SceneController_Play::BindButton(const std::string& name, engine::UIButton::ClickCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnClick([cb]() {engine::SoundSystem::Get().PlayUI("UI_Click");
        if (cb) cb(); });
    }

    void SceneController_Play::BindButton(const std::string& name, engine::UIButton::HoverCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnHover([cb](bool isHovered) {
            if (isHovered)
            {
                engine::SoundSystem::Get().PlayUI("UI_Horver");
            }
            if (cb) cb(isHovered);
            });
    }

    void SceneController_Play::BindSlider(const std::string& name, engine::UISlider::ValueChangedCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* slider = go->GetComponent<engine::UISlider>();
        if (!slider) return;

        slider->SetOnValueChanged(std::move(cb));
    }

    void SceneController_Play::SetMenuOpen(bool open)
    {
        m_isMenuOpen = open;

        const bool shouldStop = (m_isOptionOpen || m_isMenuOpen || m_isGiveupOpen);
        if (shouldStop) TimeScaler::StopWorld();
        else            TimeScaler::PlayWorld();

        if (m_aimMode) m_aimMode->SetPaused(shouldStop);

        if (m_menuPopUp)
        {
            if (auto* anim = m_menuPopUp->GetComponent<game::UIPopUpAnimator>())
            {
                open ? anim->Open() : anim->Close();

                std::string soundName = open ? "UI_Open" : "UI_Close";
                engine::SoundSystem::Get().PlayUI(soundName);
            }
            else
            {
                m_menuPopUp->SetActive(open);
            }
        }

        UpdateBlocker();
    }

    void SceneController_Play::OpenMenu()
    {
        SetMenuOpen(true);
    }

    void SceneController_Play::OpenOption()
    {
        SetOptionOpen(true);
        SetMenuOpen(false);
    }

    void SceneController_Play::BackToPlay()
    {
        SetMenuOpen(false);
    }

    void SceneController_Play::CheckBackToMain(bool open)
    {
        m_isGiveupOpen = open;
        m_realGiveupPopUp->SetActive(open);

        UpdateBlocker();
    }

    void SceneController_Play::BackToMain()
    {
        GameScene::Change(SceneID::Main);
    }

    void SceneController_Play::BackToLobby()
    {
        if (m_deathCrystalInstance)
        {
            m_deathCrystalInstance->Destroy();
            m_deathCrystalInstance = nullptr;
        }
        StageManager::Get().ResetFragileGauge();  // 로비로 복귀 시 프레자일 게이지 초기화
        //StageManager::Get().ResetRunHp(100.0f);   // 재시작시 HP 초기화 (하드코딩 제거)
        GameScene::Change(SceneID::Lobby);
    }

    void SceneController_Play::BackToRestart()
    {
        StageManager::Get().ResetFragileGauge();  // 재시작 시 프레자일 게이지 초기화
        //StageManager::Get().ResetRunHp(100.0f);   // 재시작시 HP 초기화 (하드코딩 제거)
        GameScene::Change(SceneID::Play);
    }

    void SceneController_Play::Back()
    {
        SetOptionOpen(false);
        SetMenuOpen(true);
    }

    void SceneController_Play::Fail()
    {
        if (m_isDead) return; // 중복 호출 방지

        m_isMenuOpen = false;
        m_isOptionOpen = false;
        m_isGiveupOpen = false;
        if (m_menuPopUp) m_menuPopUp->SetActive(false);
        if (m_optionPopUp) m_optionPopUp->SetActive(false);
        if (m_realGiveupPopUp) m_realGiveupPopUp->SetActive(false);
        if (m_blocker) m_blocker->SetActive(false);

        m_isDead = true;
        m_failElapsedSec = 0.0f;
        m_failReturnTriggered = false;

        // ─── 사망 연출 시작 ───
        m_deathCinematicActive = true;
        m_deathCinematicElapsed = 0.0f;
        m_deathCinematicDone = false;
        m_deathCrystalBurstTriggered = false;
        // 1) 플레이어 입력 차단
        if (m_playerScript)
            m_playerScript->SetControlLocked(true);

        // 2) 애니메이션 프리즈 (속도 0)
        if (m_playerAnimator)
            m_playerAnimator->SetLayerSpeed(0, 0.0f);

        // 3) 에임 모드 비활성
        if (m_aimMode)
            m_aimMode->SetPaused(true);

        // 3-1) PlayerAnimMesh 회전 스크립트 비활성화 (가시성은 유지)
        if (auto* playerAnimMesh = engine::GameObject::Find("PlayerAnimMesh"))
        {
            if (auto* aimMeshCtrl = playerAnimMesh->GetComponent<PlayerAimMeshController>())
                aimMeshCtrl->SetActive(false);
        }

        // 4) 카메라 줌인 시작 (사망 연출 듀레이션만큼)
        if (m_mainCamera && m_mainCamera->GetTransform())
        {
            m_deathCameraStartPos = m_mainCamera->GetTransform()->GetWorldPosition();
        }
        // cameraEffect와 충돌하지 않도록 정지
        if (m_cameraEffect)
            m_cameraEffect->StopEffect();

        // 5) 플레이어 사망 연출용 크리스탈 생성 (FragileCrystal_<Color>)
        if (!m_deathCrystalInstance && m_playerScript && m_playerScript->GetTransform())
        {
            const std::string colorSuffix = GetDeathCrystalColorSuffix(StageManager::Get().GetCurrentMapPrefabName());
            m_deathCrystalColorSuffix = colorSuffix;
            const std::string crystalPrefab = std::string("FragileCrystal_") + colorSuffix;

            engine::GameObject* crystalGO = engine::Prefab::Instantiate(crystalPrefab);
            if (crystalGO)
            {
                m_deathCrystalInstance = crystalGO;
                if (auto* tr = crystalGO->GetTransform())
                {
                    // 플레이어의 현재 XZ 위치에 독립 배치 (부모 연결 없음)
                    const engine::Vector3 playerPos = m_playerScript->GetTransform()->GetWorldPosition();
                    engine::Vector3 crystalPos = tr->GetWorldPosition();
                    crystalPos.x = playerPos.x;
                    crystalPos.z = playerPos.z;
                    tr->SetLocalPosition(crystalPos);

                    // 내부 IceFill 컨트롤러를 찾아 연출 시간과 동기화
                    if (auto* fill = FindCrystalFillInHierarchy(tr))
                    {
                        fill->SetDuration(m_deathCinematicDuration);
                        fill->SetStepCount(20);
                    }
                }
            }
        }

        // 월드는 멈추지 않는다 (연출이 재생되어야 하므로)
        TimeScaler::PlayWorld();

        // FailPanel은 아직 표시하지 않음 — 연출 완료 후에 표시
    }

    void SceneController_Play::UpdateBlocker()
    {
        if (!m_blocker) return;
        const bool isAnyPopupOpen = (m_isMenuOpen || m_isOptionOpen || m_isGiveupOpen);

        m_blocker->SetActive(isAnyPopupOpen);

        if (isAnyPopupOpen)
            TimeScaler::StopWorld();
        else
            TimeScaler::PlayWorld();

        auto* aimGO = engine::GameObject::Find("Player");

        if (aimGO)
        {
            if (auto* amc = aimGO->GetComponent<AimModeController>())
            {
                amc->SetPaused(isAnyPopupOpen);
            }
        }
    }

    void SceneController_Play::OnBGMChanged(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.audio.bgm = Clamp(v, 0.0f, 1.0f);

        app.SetUserSettings(s);
    }

    void SceneController_Play::OnSFXChanged(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.audio.sfx = Clamp(v, 0.0f, 1.0f);

        app.SetUserSettings(s);
    }

    void SceneController_Play::SetSensitivity(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.controls.mouseSensitivity = SliderToSensitivity(v);

        app.SetUserSettings(s);
    }

    void SceneController_Play::SetOptionOpen(bool open)
    {
        m_isOptionOpen = open;

        const bool shouldStop = (m_isOptionOpen || m_isMenuOpen || m_isGiveupOpen);
        if (shouldStop) TimeScaler::StopWorld();
        else            TimeScaler::PlayWorld();

        if (m_aimMode) m_aimMode->SetPaused(shouldStop);

        if (m_optionPopUp)
        {
            if (auto* anim = m_optionPopUp->GetComponent<game::UIPopUpAnimator>())
            {
                open ? anim->Open() : anim->Close();

                std::string soundName = open ? "UI_Open" : "UI_Close";
                engine::SoundSystem::Get().PlayUI(soundName);
            }
            else
            {
                m_optionPopUp->SetActive(open);
            }
        }

        UpdateBlocker();
    }
}