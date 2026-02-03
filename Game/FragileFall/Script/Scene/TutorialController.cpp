#include "GamePCH.h"
#include "TutorialController.h"
#include "Script/UI/UIMessageQueue.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Core/System/Input.h>


namespace game
{
    void TutorialController::Awake()
    {
        if (auto* go = engine::GameObject::Find("Canvas_Message"))
            m_queue = go->GetComponent<UIMessageQueue>();
    }

    void TutorialController::Start()
    {
        m_texts = {
           "WASD로 이동해보세요.\nShift로 대시가 가능합니다.",

           "왼쪽 하단 프레자일 게이지는\n전투중일 때 지속적으로 상승하며\n100 % 달성시 사망합니다.",

           "좌클릭을 눌러 사격하여\n몬스터를 쓰러트리세요.",
           
           "특정 몬스터는 즉시 사망하지 않고\n프레자일 상태가 됩니다.",
           "프레자일 상태가 된 몬스터에게\n가까이 다가가여 조준점을 올리면\nUI가 출력됩니다.",
           "UI가 나온다면 우클릭을 통해\n순간이동하며 몬스터를 완전히\n죽일 수 있습니다.",

           "스테이지에 있는 모든 몬스터를 처치하면\n프레자일 게이지 상승이 멈추고\n보상과 함께 탈출구와 진입로가 개방됩니다. ",
           "현재 프레자일 게이지가\n위험한 상태니 복귀를 합시다.",

           "해당 장소는 로비창으로\n 게임을 시작하거나 캐릭터를 강화하여\n더 깊은 심연으로 갈 수 있습니다.",

            "상단의 강화 탭에서\n기술을 클릭해보세요.",

            "첫번째 스킬을 클릭해봅시다.",

            "스킬에는 다양한 재화가 필요하며\n전추 보상으로 획득이 가능합니다.\n강화를 눌러 강화해봅시다.",

            "닫기버튼을 눌러 로비로 돌아가\n게임을 시작해봅시다."
        };

        ShowCurrent();
    }

    void TutorialController::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::Space))
        {
            Next();
        }
    }

    void TutorialController::ShowCurrent()
    {
        if (!m_queue) return;
        if (m_index < 0 || m_index >= (int)m_texts.size()) return;

        m_queue->PushMessage(
            UIMessageChannel::Tutorial,
            m_texts[m_index]
        );
    }

    void TutorialController::Next()
    {
        m_index++;
        ShowCurrent();
    }

    void TutorialController::OnGui()
    {

    }

    void TutorialController::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void TutorialController::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}