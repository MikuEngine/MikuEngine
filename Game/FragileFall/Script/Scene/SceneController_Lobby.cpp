#include "GamePCH.h"
#include "SceneController_Lobby.h"

#include <Core/App/AppContext.h>
#include <Core/App/WinApp.h>

#include <Framework/System/SystemManager.h>
#include <Framework/System/UIEventSystem.h>

#include <Framework/System/SoundSystem.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/UI/UIText.h>
#include <Framework/Object/Component/RectTransform.h>
#include "Script/UI/UIPopUpAnimator.h"

#include "Manager/UpgradeProgressManager.h"
#include "Script/UpgradeController.h"
#include <Manager/LoadingScreenDrawer.h>
#include "Manager/StageManager.h"
#include <Framework/Object/Component/Animator/SkeletalAnimator.h>

#include "Manager/TimeScaler.h"

#include "Script/LobbyInteraction.h"

#include "Scene/GameScene.h"
#include <Framework/Object/Component/UI/UIScrollView.h>

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

        static float Lerp(float a, float b, float t)
        {
            return a + (b - a) * t;
        }

        static engine::Vector3 LerpVec3(const engine::Vector3& a, const engine::Vector3& b, float t)
        {
            return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t), Lerp(a.z, b.z, t) };
        }

        static float DeltaAngleDeg(float a, float b)
        {
            float d = fmodf(b - a, 360.0f);
            if (d > 180.0f) d -= 360.0f;
            if (d < -180.0f) d += 360.0f;
            return d;
        }

        static float LerpAngleDeg(float a, float b, float t)
        {
            return a + DeltaAngleDeg(a, b) * t;
        }

        static engine::Vector3 LerpEulerDeg(const engine::Quaternion& a, const engine::Quaternion& b, float t)
        {
            return {
                LerpAngleDeg(a.x, b.x, t),
                LerpAngleDeg(a.y, b.y, t),
                LerpAngleDeg(a.z, b.z, t)
            };
        }

        static float SmoothStep01(float t)
        {
            t = Clamp(t, 0.f, 1.f);
            return t * t * (3.f - 2.f * t);
        }

        static engine::Vector3 Bezier2(const engine::Vector3& p0,
            const engine::Vector3& p1,
            const engine::Vector3& p2,
            float t)
        {
            float u = 1.0f - t;
            float uu = u * u;
            float tt = t * t;
            return p0 * uu + p1 * (2.0f * u * t) + p2 * tt;
        }

        // p0->p2를 XZ 평면으로 보고, 수직방향(perp)으로 휘게 만드는 컨트롤 포인트 생성
        static engine::Vector3 MakeBezierControlPointXZ(const engine::Vector3& p0,
            const engine::Vector3& p2,
            float curvature01,   // 0~1
            float turnSign)      // +1 or -1
        {
            engine::Vector3 d = { p2.x - p0.x, 0.0f, p2.z - p0.z };
            float len = sqrtf(d.x * d.x + d.z * d.z);
            if (len < 1e-6f)
                return (p0 + p2) * 0.5f;

            d.x /= len; d.z /= len;

            // perp: (-z, x)
            engine::Vector3 perp = { -d.z, 0.0f, d.x };

            engine::Vector3 mid = (p0 + p2) * 0.5f;

            // 커브 세기: 거리 비례 (0.15~0.35 추천)
            float offset = curvature01 * len * 0.5f;

            engine::Vector3 p1 = mid + perp * (offset * turnSign);
            p1.y = p0.y;
            return p1;
        }

        static engine::Vector3 Bezier2Derivative(const engine::Vector3& p0,
            const engine::Vector3& p1,
            const engine::Vector3& p2,
            float t)
        {
            // B'(t) = 2(1-t)(p1-p0) + 2t(p2-p1)
            float u = 1.0f - t;
            return (p1 - p0) * (2.0f * u) + (p2 - p1) * (2.0f * t);
        }

        static float YawDegFromDirXZ(const engine::Vector3& dir)
        {
            // 만약 좌/우가 반대로 돌면 atan2f(-dir.x, dir.z) 또는 atan2f(dir.x, -dir.z)로만 바꾸면 바로 맞습니다.
            return engine::ToDegree(atan2f(dir.x, dir.z));
        }
    }

    void SceneController_Lobby::Awake()
    {
        if (m_bound) return;
        m_bound = true;

        auto* clickGO = engine::GameObject::Find("UIClickArea");
        m_interaction = clickGO->GetComponent<LobbyInteraction>();

        BindButton("UI_EnterPlay", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->EnterPlay(); });
        BindButton("UI_OpenUpgrade", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->OpenUpgrade(); });
        BindButton("UI_OpenOption", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->OpenOption(); });
        BindButton("UI_BackToMain", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->BackToMain(); });
        BindButton("UI_BackToHub", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->BackToHub(); });
        BindButton("UI_CloseButton_Option", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->Back(); });
        BindButton("UI_CloseButton_Upgrade", [self = engine::Ptr<SceneController_Lobby>(this)]() {if (self) self->BackToHub(); });

        BindButton("UI_EnterPlay", [self = engine::Ptr<SceneController_Lobby>(this)](bool hovered) {if (self) self->ShowEffect("UI_EnterPlay", hovered); });
        BindButton("UI_OpenUpgrade", [self = engine::Ptr<SceneController_Lobby>(this)](bool hovered) {});
        BindButton("UI_OpenOption", [self = engine::Ptr<SceneController_Lobby>(this)](bool hovered) {});
        BindButton("UI_BackToMain", [self = engine::Ptr<SceneController_Lobby>(this)](bool hovered) {});
        BindButton("UI_BackToHub", [self = engine::Ptr<SceneController_Lobby>(this)](bool hovered) {});
        BindButton("UI_CloseButton_Option", [self = engine::Ptr<SceneController_Lobby>(this)](bool hovered) {});
        BindButton("UI_CloseButton_Upgrade", [self = engine::Ptr<SceneController_Lobby>(this)](bool hovered) {});

        // Sliders
        BindSlider("UI_BGMSlider", [self = engine::Ptr<SceneController_Lobby>(this)](float v) {if (self) self->OnBGMChanged(v); });
        BindSlider("UI_SFXSlider", [self = engine::Ptr<SceneController_Lobby>(this)](float v) {if (self) self->OnSFXChanged(v); });
        BindSlider("UI_SensitivitySlider", [self = engine::Ptr<SceneController_Lobby>(this)](float v) {if (self) self->SetSensitivity(v); });
    }

    void SceneController_Lobby::Start()
    {
        if (!TimeScaler::IsActive())
            TimeScaler::PlayWorld();

        auto* clickGO = engine::GameObject::Find("UIClickArea");
        m_interaction = clickGO->GetComponent<LobbyInteraction>();

        m_optionPopUp = engine::GameObject::Find("UI_OptionPopUp");
        if (m_optionPopUp) m_optionPopUp->SetActive(false);

        m_blocker = engine::GameObject::Find("Panel_Blocker");
        if (m_blocker) m_blocker->SetActive(false);

        m_groupSelect = engine::GameObject::Find("UIGroup_Select");
        if (m_groupSelect) m_groupSelect->SetActive(true);

        m_upgradePopUp = engine::GameObject::Find("UI_UpgradePopUp");
        if (m_upgradePopUp) m_upgradePopUp->SetActive(false);

        if (auto* go = engine::GameObject::Find("UI_ScrollView"))
            m_upgradeScroll = go->GetComponent<engine::UIScrollView>();

        m_isOptionOpen = false;
        m_isUpgradeOpen = false;

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

        // 플레이어 애니메이션 재생 ================================================
        m_playerPreview = engine::GameObject::Find("PlayerPreview");
        if (!m_playerPreview) return;

        m_isPlayerMove = false;

        auto* playerAnim = m_playerPreview->GetComponent<engine::SkeletalAnimator>();
        if (playerAnim) playerAnim->Play("Idle", true);

        // Load =======================================================
        auto* ugdGO = engine::GameObject::Find("UpgradeController");
        if (!ugdGO) return;

        auto* uc = ugdGO->GetComponent<game::UpgradeController>();
        if (!uc) return;

        game::UpgradeProgressManager::LoadProgress(*uc);

        // bgm sound
        engine::SoundSystem::Get().PlayBGM("Lobby_BGM");
    }

    void SceneController_Lobby::Update()
    {
        if (m_upgradeTransition || m_optionTransition)
        {
            m_uiTransitionTimer += engine::Time::UnscaledDeltaTime();

            // 애니 길이에 맞춰 값 조절
            if (m_uiTransitionTimer >= 0.15f)
            {
                m_upgradeTransition = false;
                m_optionTransition = false;
                m_uiTransitionTimer = 0.0f;
            }
        }

        if (engine::Input::IsKeyPressed(engine::Keys::Escape))
        {
            if (!m_entered)
                HandleEscape();
        }

        if (!(m_playerPreview && m_isPlayerMove)) return;

        auto* tr = m_playerPreview->GetTransform();
        if (!tr) return;
        
        // 회전
        if (!m_rotDone)
        {
            m_rotElapsed += engine::Time::UnscaledDeltaTime();

            float tRot = (m_rotDuration > 0.0f) ? Clamp(m_rotElapsed / m_rotDuration, 0.f, 1.f) : 1.f;
            float sRot = SmoothStep01(tRot);

            engine::Quaternion r;
            engine::Quaternion::Slerp(m_moveStartRot, m_moveTargetRot, sRot, r);

            // 끝 프레임엔 "정확히" 목표값 고정
            if (tRot >= 1.0f)
            {
                r = m_moveTargetRot;
                m_rotDone = true;

                // 회전 끝나는 순간부터 걷기 시작
                if (!m_walkStarted)
                {
                    m_walkStarted = true;
                    if (auto* anim = m_playerPreview->GetComponent<engine::SkeletalAnimator>())
                        anim->PlayCrossFade("Walk", 0.3f, true, 0, 1);
                }
            }

            tr->SetLocalRotation(r);
            return; // 회전 중에는 이동 로직 아예 실행 안 함
        }

        // 이동
        m_moveElapsed += engine::Time::UnscaledDeltaTime();

        float tMove = (m_moveDuration > 0.0f) ? Clamp(m_moveElapsed / m_moveDuration, 0.f, 1.f) : 1.f;
        float sMove = SmoothStep01(tMove);

        // 위치
        engine::Vector3 currentPos = Bezier2(m_curveP0, m_curveP1, m_curveP2, sMove);
        tr->SetLocalPosition(currentPos);

        // 진행방향으로 회전(원하신 "가는 방향대로 돌기")
        {
            engine::Vector3 tangent = Bezier2Derivative(m_curveP0, m_curveP1, m_curveP2, sMove);
            tangent.y = 0.0f;

            const float len2 = tangent.x * tangent.x + tangent.z * tangent.z;
            if (len2 > 1e-6f)
            {
                const float invLen = 1.0f / sqrtf(len2);
                engine::Vector3 dir = { tangent.x * invLen, 0.0f, tangent.z * invLen };

                float yawDeg = YawDegFromDirXZ(dir);
                engine::Quaternion lookRot =
                    engine::Quaternion::CreateFromYawPitchRoll(engine::ToRadian(yawDeg), 0.f, 0.f);

                // 너무 빡세게 고개가 튀면 스무딩(값 낮추면 더 느릿하게 따라감)
                engine::Quaternion cur = tr->GetLocalRotation();
                engine::Quaternion smoothed;
                engine::Quaternion::Slerp(cur, lookRot, 0.2f, smoothed);
                tr->SetLocalRotation(smoothed);
            }
        }

        // 완료
        if (tMove >= 1.0f)
        {
            m_isPlayerMove = false;
            game::StageManager::Get().ResetToStage1();
            GameScene::Change(SceneID::Play);
        }
    }

    void SceneController_Lobby::OnGui()
    {
        ImGui::DragFloat3("TargetPos", &m_moveTargetPos.x);
        ImGui::DragFloat("Curve", &m_curve01);

        ImGui::DragFloat("MoveSpeed", &m_moveDuration);
    }

    void SceneController_Lobby::Save(engine::json& j) const
    {
        Object::Save(j);
        j["TargetPos"] = m_moveTargetPos;
        j["Curve"] = m_curve01;

        j["MoveSpeed"] = m_moveDuration;
    }

    void SceneController_Lobby::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "TargetPos", m_moveTargetPos);
        engine::JsonGet(j, "Curve", m_curve01);

        engine::JsonGet(j, "MoveSpeed", m_moveDuration);
    }

    void SceneController_Lobby::BindButton(const std::string& name, engine::UIButton::ClickCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnClick([cb]() {engine::SoundSystem::Get().PlayUI("UI_Click");
        if (cb) cb(); });
    }

    void SceneController_Lobby::BindButton(const std::string& name, engine::UIButton::HoverCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* button = go->GetComponent<engine::UIButton>();
        if (!button) return;

        button->AddOnHover([cb](bool isHovered){
            if (isHovered)
            {
                engine::SoundSystem::Get().PlayUI("UI_Horver");
            }
            if (cb) cb(isHovered);
            });
    }

    void SceneController_Lobby::BindSlider(const std::string& name, engine::UISlider::ValueChangedCallback cb)
    {
        auto* go = engine::GameObject::Find(name);
        if (!go) return;

        auto* slider = go->GetComponent<engine::UISlider>();
        if (!slider) return;

        slider->SetOnValueChanged(std::move(cb));
    }

    void SceneController_Lobby::EnterPlay()
    {
        if (!m_playerPreview) return;
        
        // 진입하면 ESC 등 비활성화
        m_entered = true;

        // 진입하면 입력 차단(화면은 어두워지지 않게)
        if (m_blocker)
        {
            m_blocker->SetActive(true);
            auto* img = m_blocker->GetComponent<engine::UIImage>();
            img->SetColor({ 0,0,0,0 });
        }

        // 로비에서 상호작용 되는거 비활성화
        m_interaction->SetInteractionActive(false);

        auto* tr = m_playerPreview->GetTransform();
        if (!tr) return;

        m_moveStartPos = tr->GetLocalPosition();
        m_moveStartRot = tr->GetLocalRotation();

        float targetYawDeg = 43.272f;
        m_moveTargetRot = engine::Quaternion::CreateFromYawPitchRoll(engine::ToRadian(targetYawDeg), 0.f, 0.f);

        m_curveP0 = m_moveStartPos;
        m_curveP2 = m_moveTargetPos;

        float startYawDeg = -159.925f;
        float dyaw = DeltaAngleDeg(startYawDeg, targetYawDeg);
        float turnSign = (dyaw >= 0.0f) ? +1.0f : -1.0f;
        m_curveP1 = MakeBezierControlPointXZ(m_curveP0, m_curveP2, m_curve01, turnSign);

        m_rotDone = false;
        m_walkStarted = false;
        m_rotElapsed = 0.0f;
        m_moveElapsed = 0.0f;

        m_isPlayerMove = true;
    }

    void SceneController_Lobby::OpenUpgrade()
    {
        SetUpgradeOpen(true);
    }

    void SceneController_Lobby::OpenOption()
    {
        SetOptionOpen(true);
    }

    void SceneController_Lobby::BackToMain()
    {
        GameScene::Change(SceneID::Main);
    }

    void SceneController_Lobby::BackToHub()
    {
        SetUpgradeOpen(false);
    }

    void SceneController_Lobby::SetOptionOpen(bool open)
    {
        m_isOptionOpen = open;

        // 로비에서 상호작용 되는거 비활성화
        bool shouldEnableInteraction = !m_isOptionOpen && !m_isUpgradeOpen && !m_entered;
        if (m_interaction)
            m_interaction->SetInteractionActive(shouldEnableInteraction);

        const bool shouldStop = (m_isOptionOpen || m_isUpgradeOpen);
        if (shouldStop) TimeScaler::StopWorld();
        else            TimeScaler::PlayWorld();

        if (m_blocker)
            m_blocker->SetActive(open);

        if (!m_optionPopUp) return;
           
        if (auto* anim = m_optionPopUp->GetComponent<game::UIPopUpAnimator>())
        {
            m_optionTransition = true;
            open ? anim->Open() : anim->Close();

            std::string soundName = m_isOptionOpen ? "UI_Open" : "UI_Close";
            engine::SoundSystem::Get().PlayUI(soundName);
        }
        else
        {
            m_optionPopUp->SetActive(open);
            m_optionTransition = false;
        }
    }

    void SceneController_Lobby::SetUpgradeOpen(bool open)
    {
        m_isUpgradeOpen = open;

        // 로비에서 상호작용 되는거 비활성화
        bool shouldEnableInteraction = !m_isOptionOpen && !m_isUpgradeOpen && !m_entered;
        if (m_interaction)
            m_interaction->SetInteractionActive(shouldEnableInteraction);

        const bool shouldStop = (m_isOptionOpen || m_isUpgradeOpen);
        if (shouldStop) TimeScaler::StopWorld();
        else            TimeScaler::PlayWorld();

        if (m_blocker)
            m_blocker->SetActive(open);

        if (!open)
        {
            engine::UIEventSystem& ui =
                engine::SystemManager::Get().GetUIEventSystem();

            ui.ResetPointerState(true);
            ui.MarkDirty();

            if (m_upgradeScroll)
                m_upgradeScroll->SetScrollY(0.0f);
            
            auto* ugdGO = engine::GameObject::Find("UpgradeController");
            if (ugdGO)
            {
                if (auto* uc = ugdGO->GetComponent<game::UpgradeController>())
                {
                    // 또는 안전하게 함수를 호출 (추천)
                    uc->ResetSelection();
                }
            }
        }

        if (!m_upgradePopUp) return;

        if (auto* anim = m_upgradePopUp->GetComponent<game::UIPopUpAnimator>())
        {
            m_upgradeTransition = true;
            open ? anim->Open() : anim->Close();

            std::string soundName = m_isUpgradeOpen ? "UI_Open" : "UI_Close";
            engine::SoundSystem::Get().PlayUI(soundName);
        }
        else
        {
            m_upgradePopUp->SetActive(open);
            m_upgradeTransition = false;
        }
    }

    void SceneController_Lobby::HandleEscape()
    {
        if (m_upgradeTransition || m_optionTransition)
            return;

        if (m_isOptionOpen)
        {
            SetOptionOpen(false);
            engine::SoundSystem::Get().PlayUI("UI_Click");
            return;
        }

        if (m_isUpgradeOpen)
        {
            SetUpgradeOpen(false);
            engine::SoundSystem::Get().PlayUI("UI_Click");
            return;
        }
    }

    void SceneController_Lobby::Back()
    {
        engine::AppContext::GetApp().SaveUserSettings();
        SetOptionOpen(false);
    }

    void SceneController_Lobby::ShowEffect(const std::string& targetName, bool hovered)
    {
        auto* targetGO = engine::GameObject::Find(targetName);
        if (!targetGO) return;

        auto img = targetGO->GetComponent<engine::UIImage>();
        if (!img) return;

        if (hovered)
        {
            img->SetEffect(engine::UIEffectType::AbyssalDecay);
        }
        else
        {
            img->ClearEffect();
        }
    }

    void SceneController_Lobby::OnBGMChanged(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.audio.bgm = Clamp(v, 0.0f, 1.0f);

        app.SetUserSettings(s);
    }

    void SceneController_Lobby::OnSFXChanged(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.audio.sfx = Clamp(v, 0.0f, 1.0f);

        app.SetUserSettings(s);
    }

    void SceneController_Lobby::SetSensitivity(float v)
    {
        auto& app = engine::AppContext::GetApp();

        engine::UserSettings s = app.GetUserSettings();
        s.controls.mouseSensitivity = SliderToSensitivity(v);

        app.SetUserSettings(s);
    }
}