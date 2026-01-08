#pragma once

#include "Framework/Object/Component/Animator.h"

namespace engine
{
	class SpriteData;
	class SpriteAnimationData;
	class SpriteRenderer;

	class SpriteAnimator :
		public Animator
	{
		REGISTER_COMPONENT(SpriteAnimator)

	private:
		std::string m_spriteDataPath;
		std::vector<std::string> m_animDataPaths;

		std::shared_ptr<SpriteData> m_spriteData;
		std::vector<std::shared_ptr<SpriteAnimationData>> m_animDatas;
		std::unordered_map<std::string, size_t> m_animIndexMap;
		SpriteRenderer* m_spriteRenderer = nullptr;

		float m_animationProgressTime = 0.0f;
		float m_playSpeed = 1.0f;
		size_t m_frameCounter = 0;
		size_t m_eventCounter = 0;
		int m_currentAnimIndex = -1;

		bool m_isLoop = false;
		bool m_finished = true;

	public:
		void Initialize() override;
		void Awake() override;

		void SetSpriteData(const std::string& path);
		void AddAnimationData(const std::string& path);
		void RemoveAnimationData(const std::string& path);

		void Play(int index, bool loop = true);
		void Play(const std::string& animationName, bool loop = true);

		void Update() override;

		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;
		std::string GetType() const override;
	};
}