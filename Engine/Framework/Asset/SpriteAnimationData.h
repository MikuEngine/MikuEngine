#pragma once

#include "Framework/Asset/AssetData.h"

namespace engine
{
	class SpriteData;

	struct SpriteFrameInfo
	{
		std::string pieceName;
		size_t pieceIndex;
		float time;
	};
	
	struct SpriteEventInfo
	{
		std::string name;
		std::string parameter;
		float time;
	};

	class SpriteAnimationData :
		public AssetData
	{
	private:
		std::string m_name;
		std::vector<SpriteFrameInfo> m_frames;
		std::vector<SpriteEventInfo> m_events;
		float m_duration = 0.0f;
		bool m_isLoop = false;

	public:
		void Create(const std::string& filePath);

	public:
		float GetDuration() const;
		bool IsLoop() const;
		const std::string& GetName() const;
		const std::vector<SpriteFrameInfo>& GetFrames() const;
		const std::vector<SpriteEventInfo>& GetEvents() const;

		void SetupFramesIndex(const SpriteData* spriteData);
	};
}