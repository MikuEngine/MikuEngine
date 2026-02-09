#include "GamePCH.h"
#include "TutorialController.h"
#include "Script/UI/UIMessageQueue.h"

#include <Framework/Object/GameObject/GameObject.h>
//#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Asset/Prefab.h>
#include <Core/System/Input.h>

#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/UI/UIButton.h>
#include <Framework/Object/Component/UI/UIClickArea.h>
#include <Framework/Object/Component/RectTransform.h>

#include "Script/CharacterScript/Monster/MonsterScript.h"

#include "Script/DoorTriggerScript.h"

namespace game
{
    static engine::Ptr<engine::GameObject> s_tutorialController = nullptr;

    namespace
    {
        const std::vector<TutorialStep> g_steps = {
            {{"WASD로 이동해보세요.\nShift로 대시가 가능합니다."}},

            {{"왼쪽 하단 프레자일 게이지는\n전투중일 때 지속적으로 상승하며\n100 % 달성시 사망합니다."}},

            {{"좌클릭을 눌러 사격하여\n몬스터를 쓰러트리세요."}},// 1

            {{
            "특정 몬스터는 즉시 사망하지 않고\n프레자일 상태가 됩니다.",// 1
            "프레자일 상태가 된 몬스터에게\n가까이 다가가여 조준점을 올리면\nUI가 출력됩니다.",// 2
            "UI가 나온다면 우클릭을 통해\n순간이동하며 몬스터를 완전히\n죽일 수 있습니다.",// 3
            }},

            {{
            "스테이지의 모든 몬스터를 처치하면\n프레자일 게이지 상승이 멈추고\n보상과 함께 탈출구와 진입로가 개방됩니다. ", //1
            "현재 프레자일 게이지가\n위험한 상태니 복귀를 합시다.",//2
            }},

            {{"해당 장소는 로비창으로\n게임을 시작하거나 캐릭터를 강화하여\n더 깊은 심연으로 갈 수 있습니다."}},// 1

            {{"상단의 강화 탭에서\n기술을 클릭해보세요."}},// 1

            {{"첫번째 스킬을 클릭해봅시다."}},// 1

            {{"스킬에는 다양한 재화가 필요하며\n전추 보상으로 획득이 가능합니다.\n강화를 눌러 강화해봅시다."}},// 1

            {{"닫기버튼을 눌러 로비로 돌아가\n게임을 시작해봅시다."}}// 1
        };
    }

    void TutorialController::Awake()
    {
        /*/ 강화 튜토리얼 디버깅용
        std::string currentScene = (engine::SceneManager::Get().GetScene()) ? engine::SceneManager::Get().GetScene()->GetName() : "";
        if (currentScene == "10_PROTO_TutorialLobby")
        {
            m_stepIndex = 4;
            m_pageIndex = 1;
            OnSceneLoaded();
        }
        //*/
    }

    void TutorialController::Start()
    {
        if (s_tutorialController == nullptr)
        {
            s_tutorialController = GetGameObject();
            GetGameObject()->DontDestroyOnLoad();

            if (m_player = engine::GameObject::Find("Player"))
            {
                auto pc = m_player->GetComponent<PlayerControllerScript>();
                //pc->SetBaseAtkDmg(30.0f);
            }

            m_nextDoorObject = engine::GameObject::Find("StageDoor_Next");
            m_exitDoorObject = engine::GameObject::Find("StageDoor_Exit");

            ShowPage();
        }
        else
        {
            auto* oldController = s_tutorialController->GetComponent<TutorialController>();
            if (oldController)
            {
                oldController->OnSceneLoaded();
            }

            GetGameObject()->Destroy();
        }

    }

    void TutorialController::Update()
    {
        // 단축키
        if ( engine::Input::IsKeyPressed(engine::Keys::F10))
        {
            Next();
        }
        else if (engine::Input::IsKeyPressed(engine::Keys::F9))
        {
            Prev();
        }

        if (m_isTimerActive)
        {
            m_stepTimer -= engine::Time::FixedDeltaTime();

            if (m_stepTimer <= 0.0f)
            {
                m_isTimerActive = false;

                switch (m_stepIndex)
                {

                case 0:
                    if (m_nextDoorObject)
                    {
                        m_nextDoorObject->GetComponent<DoorTriggerScript>()->SetActivateDoor(true);
                    }
                    break;
                // ─────────────────────────────────────────────
                // step 1
                // ─────────────────────────────────────────────
                case 1:
                    break;
                // ─────────────────────────────────────────────
                // step 2
                // ─────────────────────────────────────────────
                case 2:
                    if (auto spawnObject = engine::GameObject::Find("MonsterSpawnPoint"))
                    {
                        m_spawnedMonster = engine::Prefab::Instantiate(m_monsterName);
                        if (m_spawnedMonster && m_spawnedMonster->GetTransform())
                        {
                            m_spawnedMonster->GetTransform()->SetWorldMatrix(spawnObject->GetTransform()->GetWorld());
                            m_isSpawnMonster = true;                      
                        }
                    }
                    break;
                // ─────────────────────────────────────────────
                // step 3
                // ─────────────────────────────────────────────
                case 3:
                    if (m_pageIndex >= 2)
                    {
                        m_keepMonsterOnNext = false;
                    }
                    if (m_pageIndex < (int)g_steps[m_stepIndex].pages.size() - 1)
                    {
                        Next();
                        m_isTimerActive = true;
                        m_stepTimer = 3.0f;
                    }
                    break;
                // ─────────────────────────────────────────────
                // step 4
                // ─────────────────────────────────────────────
                case 4:
                    if (m_pageIndex == 0 && !m_isStateCheck)
                    {
                        m_nextDoorObject->GetComponent<DoorTriggerScript>()->SetActivateDoor(true);
                        m_exitDoorObject->GetComponent<DoorTriggerScript>()->SetActivateDoor(true);

                        m_isStateCheck = true;

                        m_isTimerActive = true;
                        m_stepTimer = 5.0f;
                    }
                    else if (m_pageIndex == 0 && m_isStateCheck)
                    {
                        Next();
                    }
                    break;
                }
            }
        }

        if (m_isSpawnMonster)
        {
            if (m_spawnedMonster && m_monsterName == "Monster_PointedType_Gray" && !m_isStateCheck)
            {
                if (m_spawnedMonster->GetComponent<MonsterScript>()->IsFragile())
                {
                    m_isStateCheck = true;
                    m_keepMonsterOnNext = true;
                    Next();
                    m_isTimerActive = true;
                    m_stepTimer = 3.0f;
                }
            }

            if (m_spawnedMonster == nullptr || m_spawnedMonster->IsPendingKill())
            {
                m_isSpawnMonster = false;
                m_keepMonsterOnNext = false;
                m_spawnedMonster = nullptr;

                if (m_monsterName == "Monster_DullType_Gray")
                {
                    m_monsterName = "Monster_PointedType_Gray";
                    m_isTimerActive = true;
                    m_stepTimer = 8.0f;
                }
                else if (m_monsterName == "Monster_PointedType_Gray")
                {
                    Next();
                }
            }
        }
    }

    void TutorialController::InitializeStep()
    {
        if (m_keepMonsterOnNext) return;

        m_stepTimer = 0.0f;
        m_isTimerActive = false;
        m_isStateCheck = false;
        
        if (m_nextDoorObject)
        {
            m_nextDoorObject->GetComponent<DoorTriggerScript>()->SetActivateDoor(false);
            m_exitDoorObject->GetComponent<DoorTriggerScript>()->SetActivateDoor(false);
        }
 
        // 몬스터 초기화
        if (m_spawnedMonster)
        {
            if (m_keepMonsterOnNext)
            {
                m_keepMonsterOnNext = false;
                return;
            }

            m_spawnedMonster->Destroy();
            m_spawnedMonster = nullptr;
            m_isSpawnMonster = false;
            m_monsterName = "Monster_DullType_Gray";
        }

    }

    void TutorialController::RefreshStepContext(int index)
    {
        InitializeStep();

        switch (index)
        {
            // ─────────────────────────────────────────────
            // step 0 : 다음 맵 진입
            // ─────────────────────────────────────────────
        case 0:
            m_isTimerActive = true;
            m_stepTimer = 8.0f;
            break;
            // ─────────────────────────────────────────────
            // step 1 :
            // ─────────────────────────────────────────────
        case 1:
            break;
            // ─────────────────────────────────────────────
            // step 2 : 일반 몬스터(회색 둔탁) 1종을 소환하여 사격하게 UI로 유도
            //          뾰족 몬스터를 사격 프레자일 상태로 만들도록 유도
            // ─────────────────────────────────────────────
        case 2:
            m_isTimerActive = true;
            m_stepTimer = 5.0f;
            break;
            // ─────────────────────────────────────────────
            // step 3 : 몬스터가 프레자일 상태가 될 경우 UI 변경후 N초후 순차대로 출력
            // ─────────────────────────────────────────────
        case 3:
            break;
            // ─────────────────────────────────────────────
            // step 4 : 처형시 다음맵으로 가는문 개방
            //          처형시 설명 UI를 먼저 띄워주고 N초 뒤로비로 가는 문 2종 개방 후 복귀를 유도
            // ─────────────────────────────────────────────
        case 4:
            m_isTimerActive = true;
            m_stepTimer = 0.3f;
            break;
            // ─────────────────────────────────────────────
            // step 5 : 로비 UI 진입시 설명창을 띄워주며 강화 버튼을 누르라는 UI 표시
            // ─────────────────────────────────────────────
        case 5:
            BindButton("UI_OpenUpgrade", [this]() {
                SetActiveButton("UI_OpenUpgrade", false, false);
                Next();
            });
            break;
            // ─────────────────────────────────────────────
            // step 6 : 기술 부분 클릭 유도
            // ─────────────────────────────────────────────
        case 6:
            BindButton("Btn_Skill", [this]() {
                SetActiveButton("Btn_Skill", false, false);
                Next();
            });
            break;
            // ─────────────────────────────────────────────
            // step 7 : 기술에 첫 번째 스킬 처형(브레이크) 시 프레자일 게이지 회복
            // ─────────────────────────────────────────────
        case 7:
            SetupSkillNodesUI();
            break;
            // ─────────────────────────────────────────────
            // step 8 : 재화가 들어간다는 설명과 함께 강화 유도
            // ─────────────────────────────────────────────
        case 8:
            {
            auto upgradeUI = engine::GameObject::Find("Desc_Panel");
            if (upgradeUI)
            {
                upgradeUI->GetComponent<engine::UIImage>()->SetOutline(true, m_outlineWidth, { 1.0f, 0.0f, 0.0f, 1.0f });
            }
            BindButton("Btn_Upgrade", [this, upgradeUI] () {
                this->SetActiveButton("Btn_Upgrade", false, false);
                if (upgradeUI)
                {
                    upgradeUI->GetComponent<engine::UIImage>()->SetOutline(false, m_outlineWidth, { 1.0f, 0.0f, 0.0f, 1.0f });
                }
                this->Next();
            });
            break;
            }
            // ─────────────────────────────────────────────
            // step 9 : 강화를 성공하면 x버튼을 눌러서 다시 로비로 가서 본 게임 시작
            // ─────────────────────────────────────────────
        case 9:
            BindButton("UI_CloseButton_Upgrade", [this]() {
                SetActiveButton("UI_CloseButton_Upgrade", false, false);
				SetActiveButton("UI_EnterPlay", true, true);
            });
            break;
        }
    }

    void TutorialController::OnSceneLoaded()
    {
        m_queue = nullptr;
        m_player = nullptr;
        m_nextDoorObject = nullptr;
        m_exitDoorObject = nullptr;

        auto* go = engine::GameObject::Find("Canvas_Message");
        if (go) m_queue = go->GetComponent<UIMessageQueue>();

        std::string currentScene = (engine::SceneManager::Get().GetScene()) ? engine::SceneManager::Get().GetScene()->GetName() : "";

        if (currentScene == "10_PROTO_Tutorial")
        {
            m_player = engine::GameObject::Find("Player");

            if (m_player)
            {
                auto pc = m_player->GetComponent<PlayerControllerScript>();
                //if (pc) pc->SetBaseAtkDmg(30.0f);
            }

            m_nextDoorObject = engine::GameObject::Find("StageDoor_Next");
            m_exitDoorObject = engine::GameObject::Find("StageDoor_Exit");
             
            Next();
        }
        else if (currentScene == "10_PROTO_TutorialLobby")
        {
            Next();
        }
    }

    void TutorialController::BindButton(const std::string& goName, std::function<void()> callback)
    {
        if (auto* go = engine::GameObject::Find(goName))
        {
            if (auto* btn = go->GetComponent<engine::UIButton>())
            {
				btn->SetActive(true);
                btn->AddOnClick(std::move(callback));
            }
        }

        auto upgradeUI = engine::GameObject::Find(goName);
        if (upgradeUI)
        {
            upgradeUI->GetComponent<engine::UIImage>()->SetOutline(true, m_outlineWidth, { 1.0f, 0.0f, 0.0f, 1.0f });
        }
    }

    void TutorialController::SetActiveButton(const std::string& goName, bool active, bool outline)
    {
        if (auto* go = engine::GameObject::Find(goName))
        {
            if (auto* btn = go->GetComponent<engine::UIButton>())
            {
                btn->SetActive(active);
            }
        }

        auto upgradeUI = engine::GameObject::Find(goName);
        if (upgradeUI)
        {
            upgradeUI->GetComponent<engine::UIImage>()->SetOutline(outline, m_outlineWidth, { 1.0f, 0.0f, 0.0f, 1.0f });
        }
    }

    void TutorialController::SetupSkillNodesUI()
    {
        engine::GameObject* parentGo = engine::GameObject::Find("SkillNodes");
        if (!parentGo) return;

        engine::RectTransform* parentRT = parentGo->GetComponent<engine::RectTransform>();
        if (!parentRT) return;

        const std::vector<engine::Transform*>& children = parentRT->GetChildren();

        if (!children.empty())
        {
            engine::GameObject* firstNode = children[0]->GetGameObject();
            if (firstNode)
            {
				LOG_PRINT("Activate Skill Node UI");

                auto button = firstNode->GetComponent<engine::UIClickArea>();
                
                button->SetActive(true);
                button->AddOnClick([this, button](int buttonIndex){
                    button->SetActive(false);
                    this->Next();
                });

                firstNode->GetComponent<engine::UIImage>()->SetOutline(true, m_outlineWidth, { 1.0f, 0.0f, 0.0f, 1.0f });
            }
            else
            {
                LOG_PRINT("not found not found Activate Skill Node UI");
            }
        }
    }

    void TutorialController::ShowPage()
    {
        if (!m_queue)
        {
            if (auto* go = engine::GameObject::Find("Canvas_Message"))
                m_queue = go->GetComponent<UIMessageQueue>();
        }
        if (!m_queue) return;

        if (m_stepIndex < 0 || m_stepIndex >= (int)g_steps.size()) return;

        const auto& step = g_steps[m_stepIndex];
        if (m_pageIndex < 0 || m_pageIndex >= (int)step.pages.size()) return;

        m_queue->PushMessage(UIMessageChannel::Tutorial, step.pages[m_pageIndex]);

        RefreshStepContext(m_stepIndex);
    }

    void TutorialController::Next()
    {
        if (!m_queue)
        {
            if (auto* go = engine::GameObject::Find("Canvas_Message"))
                m_queue = go->GetComponent<UIMessageQueue>();
        }
        if (!m_queue) return;

        const auto& step = g_steps[m_stepIndex];

        // 같은 스텝 내에서 페이지를 넘길 때 -> 아래에 누적(Push)
        if (m_pageIndex < (int)step.pages.size() - 1)
        {
            m_pageIndex++;
            m_queue->PushMessage(UIMessageChannel::Tutorial, step.pages[m_pageIndex]);
            return;
        }

        // 다음 스텝으로 넘어갈 때 (초기화 후 SetSingle로 새로 시작)
        if (m_stepIndex < (int)g_steps.size() - 1)
        {
            m_stepIndex++;
            m_pageIndex = 0;

            // 컨텍스트 초기화
            RefreshStepContext(m_stepIndex);

            // 다음 스텝의 데이터를 가져옴
            const auto& nextStep = g_steps[m_stepIndex];

            // 기존 튜토리얼 채널 메시지를 즉시 지우고 새 스텝의 첫 페이지를 세팅
            m_queue->ClearChannel(UIMessageChannel::Tutorial, 0.0f);
            m_queue->SetSingle(UIMessageChannel::Tutorial, nextStep.pages[0]);
        }
        else
        {
            m_queue->ClearChannel(UIMessageChannel::Tutorial, 0.2f);
        }
    }

    void TutorialController::Prev()
    {
        if (m_stepIndex <= 0 && m_pageIndex <= 0) return;

        // 이전으로 돌아갈 때는 기존에 쌓인 걸 지우고 다시 보여주는 게 깔끔합니다.
        m_queue->ClearChannel(UIMessageChannel::Tutorial, 0.1f);

        if (m_pageIndex > 0)
        {
            m_pageIndex--;
        }
        else
        {
            m_stepIndex--;
            m_pageIndex = (int)g_steps[m_stepIndex].pages.size() - 1;
        }

        // [참고] 이전 스텝의 모든 페이지를 한꺼번에 다 Push하고 싶다면 
        // 반복문을 써야겠지만, 보통은 해당 위치의 한 페이지만 다시 보여줍니다.
        ShowPage();
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