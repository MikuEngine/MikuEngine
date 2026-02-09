#include "GamePCH.h"
#include "UISliderValueTextBinder.h"

#include <Core/App/AppContext.h>
#include <Core/App/WinApp.h>

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/UI/UIText.h>

namespace game
{
    static std::string FormatFloat(float v, int digits)
    {
        float finVal = v * 100;

        char buf[64];
        std::snprintf(buf, sizeof(buf), ("%." + std::to_string(digits) + "f").c_str(), finVal);
        return std::string(buf);
    }

    static std::string Format01OrPercent(float v01)
    {
        // 0~1 값이면 0~100%로 표시하는 예시
        const int p = (int)std::round(v01 * 100.0f);
        return std::to_string(p) + "%";
    }

    void UISliderValueTextBinder::Awake()
    {
        // Bind
        {
            auto* go = engine::GameObject::Find("Text_VolumeVal");   // BGM
            if (go) m_volumeTxt = go->GetComponent<engine::UIText>();
        }

        {
            auto* go = engine::GameObject::Find("Text_SFXVal");     // 효과음
            if (go) m_SFXTxt = go->GetComponent<engine::UIText>();
        }

        {
            auto* go = engine::GameObject::Find("Text_SensiVal");   // 마우스 감도
            if (go) m_SensiTxt = go->GetComponent<engine::UIText>();
        }
    }

    void UISliderValueTextBinder::Start()
    {
        if (!m_volumeTxt || !m_SFXTxt || !m_SensiTxt) return;

        Refresh(true);
    }

    void UISliderValueTextBinder::Update()
    {
        if (!m_volumeTxt || !m_SFXTxt || !m_SensiTxt) return;

        Refresh(false);
    }

    void UISliderValueTextBinder::OnGui()
    {

    }

    void UISliderValueTextBinder::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void UISliderValueTextBinder::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void UISliderValueTextBinder::Refresh(bool force)
    {
        auto& app = engine::AppContext::GetApp();
        engine::UserSettings s = app.GetUserSettings();

        m_volumeTxt->SetText(Format01OrPercent(s.audio.bgm));
        m_SFXTxt->SetText(Format01OrPercent(s.audio.sfx));
        m_SensiTxt->SetText(FormatFloat(s.controls.mouseSensitivity, 2));
    }
}