#include "EnginePCH.h"
#include "SpriteAnimator.h"

#include "Framework/Asset/AssetManager.h"
#include "Framework/Asset/SpriteData.h"
#include "Framework/Asset/SpriteAnimationData.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/SpriteRenderer.h"

namespace engine
{
	void SpriteAnimator::Initialize()
	{
		Animator::Initialize();

		if (!m_spriteDataPath.empty())
		{
			SetSpriteData(m_spriteDataPath);
		}

		for (const auto& path : m_animDataPaths)
		{
			if (path.empty())
			{
				continue;
			}

			auto animData = AssetManager::Get().GetOrCreateSpriteAnimationData(path);
			if (animData)
			{
				animData->SetupFramesIndex(m_spriteData.get());
				m_animDatas.push_back(animData);
			}
		}

		m_animIndexMap.clear();
		for (size_t i = 0; i < m_animDatas.size(); ++i)
		{
			m_animIndexMap[m_animDatas[i]->GetName()] = i;
		}
	}

	void SpriteAnimator::Awake()
	{
		if (m_spriteData)
		{
			m_spriteRenderer = GetGameObject()->GetComponent<SpriteRenderer>();

			// 초기 스프라이트(0번) 설정
			if (m_spriteRenderer != nullptr)
			{
				if (m_spriteData->GetSpritePiece(0).width > 0) // 유효성 체크
				{
					if (m_spriteData->GetWidth() > 0 && m_spriteData->GetHeight() > 0)
					{
						const auto& piece = m_spriteData->GetSpritePiece(0);
						const float sheetW = m_spriteData->GetWidth();
						const float sheetH = m_spriteData->GetHeight();
						const Vector2 uvOffset{ piece.x / sheetW, piece.y / sheetH };
						const Vector2 uvScale{ piece.width / sheetW, piece.height / sheetH };
						const Vector2 pivot{ piece.pivotX, piece.pivotY };
						m_spriteRenderer->SetSpriteInfo(uvOffset, uvScale, pivot);
					}
				}
			}
		}
	}

	void SpriteAnimator::SetSpriteData(const std::string& path)
	{
		m_spriteDataPath = path;
		m_spriteData = AssetManager::Get().GetOrCreateSpriteData(path);
		if (m_spriteData)
		{
			// SpriteData가 변경되었으므로 Renderer 업데이트
			if (auto renderer = GetGameObject()->GetComponent<SpriteRenderer>())
			{
				// 기본 0번 피스로 리셋
				// 실제로는 현재 애니메이션 상태에 따라 달라질 수 있으나,
				// 데이터 자체가 바뀌면 초기화하는 것이 일반적
				if (m_spriteData->GetWidth() > 0 && m_spriteData->GetHeight() > 0)
				{
					const auto& piece = m_spriteData->GetSpritePiece(0);
					const float sheetW = m_spriteData->GetWidth();
					const float sheetH = m_spriteData->GetHeight();
					const Vector2 uvOffset{ piece.x / sheetW, piece.y / sheetH };
					const Vector2 uvScale{ piece.width / sheetW, piece.height / sheetH };
					const Vector2 pivot{ piece.pivotX, piece.pivotY };
					renderer->SetSpriteInfo(uvOffset, uvScale, pivot);
				}
			}
		}
	}

	void SpriteAnimator::AddAnimationData(const std::string& path)
	{
		// 중복 체크
		for (const auto& existingPath : m_animDataPaths)
		{
			if (existingPath == path)
			{
				return;
			}
		}
		m_animDataPaths.push_back(path);
		auto animData = AssetManager::Get().GetOrCreateSpriteAnimationData(path);
		m_animDatas.push_back(animData);
		m_animIndexMap[animData->GetName()] = m_animDatas.size() - 1;
	}

	void SpriteAnimator::RemoveAnimationData(const std::string& path)
	{
		auto it = std::find(m_animDataPaths.begin(), m_animDataPaths.end(), path);
		if (it != m_animDataPaths.end())
		{
			size_t index = std::distance(m_animDataPaths.begin(), it);

			m_animDataPaths.erase(it);
			if (index < m_animDatas.size())
			{
				m_animDatas.erase(m_animDatas.begin() + index);
			}
			// Map 재구성
			m_animIndexMap.clear();
			for (size_t i = 0; i < m_animDatas.size(); ++i)
			{
				m_animIndexMap[m_animDatas[i]->GetName()] = i;
			}
		}
	}

	void SpriteAnimator::Play(int index, bool loop)
	{
		if (index < 0 || index >= m_animDatas.size())
		{
			return;
		}
		auto animData = m_animDatas[index];
		if (!animData)
		{
			return;
		}
		m_animationProgressTime = 0.0f;
		m_frameCounter = 0;
		m_eventCounter = 0;
		m_finished = false;

		m_currentAnimIndex = index;

		m_isLoop = loop;
	}

	void SpriteAnimator::Play(const std::string& animationName, bool loop)
	{
		Play(static_cast<int>(m_animIndexMap[animationName]), loop);
	}

	void SpriteAnimator::Update()
	{
		if (m_finished || m_spriteRenderer == nullptr || m_currentAnimIndex == -1)
		{
			return;
		}

		const float dt = Time::DeltaTime() * m_playSpeed;
		m_animationProgressTime += dt;
		const float duration = m_animDatas[m_currentAnimIndex]->GetDuration();
		const auto& frames = m_animDatas[m_currentAnimIndex]->GetFrames();
		// const auto& events = m_currentAnimData->GetEvents(); // 이벤트 처리 시 사용
		// 애니메이션 종료/루프 처리
		if (m_animationProgressTime >= duration)
		{
			if (m_isLoop)
			{
				m_animationProgressTime = std::fmod(m_animationProgressTime, duration);
				m_frameCounter = 0;
				m_eventCounter = 0;
			}
			else
			{
				m_animationProgressTime = duration;
				m_currentAnimIndex = -1;
				m_finished = true;
			}
		}
		// 현재 시간(m_animationProgressTime)에 맞는 프레임 찾기
		// 프레임은 시간 순으로 정렬되어 있다고 가정
		// usually frames are sorted.
		// Find frame index based on time
		size_t currentFrameIndex = m_frameCounter;

		// 간단한 선형 검색 (프레임 수가 적으므로)
		// 뒤로 갈 수도 있으니(루프 시) 0부터 검색하거나, m_frameCounter 부터 검색
		for (size_t i = 0; i < frames.size(); ++i)
		{
			if (m_animationProgressTime >= frames[i].time)
			{
				currentFrameIndex = i;
			}
			else
			{
				break;
			}
		}
		if (currentFrameIndex != m_frameCounter || m_frameCounter == 0) // 첫 프레임 or 변경 시
		{
			m_frameCounter = currentFrameIndex;
			if (frames.size() > m_frameCounter)
			{
				const auto& frameInfo = frames[m_frameCounter];

				// SpriteData에서 해당 piece 정보 가져오기
				if (m_spriteData)
				{
					// frameInfo.pieceName 혹은 pieceIndex 사용
					// SpriteData 구조체에 GetSpritePiece(index) 가 있음.

					if (m_spriteData->GetWidth() > 0 && m_spriteData->GetHeight() > 0)
					{
						const auto& piece = m_spriteData->GetSpritePiece(frameInfo.pieceIndex);

						const float sheetW = m_spriteData->GetWidth();
						const float sheetH = m_spriteData->GetHeight();
						const Vector2 uvOffset{ piece.x / sheetW, piece.y / sheetH };
						const Vector2 uvScale{ piece.width / sheetW, piece.height / sheetH };
						const Vector2 pivot{ piece.pivotX, piece.pivotY };
						m_spriteRenderer->SetSpriteInfo(uvOffset, uvScale, pivot);
					}
				}
			}
		}
		// 이벤트 처리 (생략 또는 구현)
	}

	void SpriteAnimator::OnGui()
	{
		// 1. Sprite Data 선택
		ImGui::Separator();
		ImGui::Text("Sprite Data");
		ImGui::Text("Path: %s", std::filesystem::path(m_spriteDataPath).filename().string().c_str());

		std::string selectedSpriteData;
		// .json 확장자 사용 가정 (SpriteData가 json으로 저장된다면)
		// 보통 AssetData는 .json이거나 커스텀 확장자일 수 있음. 확인 필요하나 일단 .json으로 가정.
		if (DrawFileSelector("Select SpriteData", "Resource/Data", ".json", selectedSpriteData))
		{
			SetSpriteData(selectedSpriteData);
		}
		ImGui::Spacing();
		ImGui::Separator();
		// 2. Animation Data List 관리
		ImGui::Text("Animation Datas");

		// 추가 버튼
		static std::string selectedAnimData; // static으로 선택 상태 유지 불필요하지만 DrawFileSelector 특성상 필요할수도
		if (DrawFileSelector("Add Animation", "Resource/Data", ".json", selectedAnimData))
		{
			AddAnimationData(selectedAnimData);
			selectedAnimData.clear();
		}
		ImGui::Spacing();
		// 리스트 출력 및 삭제
		if (ImGui::BeginTable("AnimDataTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 50.0f);
			ImGui::TableHeadersRow();
			// 삭제 시 인덱스 문제 방지를 위해 역순이나 별도 리스트 사용 권장되나
			// 여기서는 즉시 삭제 후 루프 탈출 (한 프레임에 하나만 삭제)
			int indexToRemove = -1;
			for (size_t i = 0; i < m_animDataPaths.size(); ++i)
			{
				ImGui::PushID(static_cast<int>(i));
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				std::string fileName = std::filesystem::path(m_animDataPaths[i]).filename().string();
				ImGui::Text("%s", fileName.c_str());
				ImGui::TableSetColumnIndex(1);
				if (ImGui::Button("X"))
				{
					indexToRemove = static_cast<int>(i);
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
			if (indexToRemove != -1)
			{
				RemoveAnimationData(m_animDataPaths[indexToRemove]);
			}
		}
	}

	void SpriteAnimator::Save(json& j) const
	{
		j["Type"] = GetType();
		j["SpriteDataPath"] = m_spriteDataPath;
		j["AnimationDataPaths"] = m_animDataPaths;
	}

	void SpriteAnimator::Load(const json& j)
	{
		JsonGet(j, "SpriteDataPath", m_spriteDataPath);
		JsonGet(j, "AnimationDataPaths", m_animDataPaths);
	}

	std::string SpriteAnimator::GetType() const
	{
		return "SpriteAnimator";
	}
}