#include "EnginePCH.h"
#include "SpriteAnimationData.h"

#include <fstream>

#include "Framework/Asset/SpriteData.h"

namespace engine
{
	void SpriteAnimationData::Create(const std::string& filePath)
	{
        std::ifstream i{ filePath };
        if (!i.is_open())
        {
            LOG_INFO("{} 파일 열기 실패", filePath);

            return;
        }

        json j;
        i >> j;
        i.close();

        m_name = j["name"];
        m_duration = j["duration"];
        m_isLoop = j["isLoop"];

        auto frames = j["frames"];
        m_frames.reserve(frames.size());

        for (const auto& jj : frames)
        {
            m_frames.emplace_back(jj["sprite"], 0LL, jj["time"]);
        }

        auto events = j["events"];
        m_events.reserve(events.size());

        for (const auto& jj : events)
        {
            m_events.emplace_back(jj["name"], jj["parameter"], jj["time"]);
        }
	}

	float SpriteAnimationData::GetDuration() const
	{
		return m_duration;
	}

	bool SpriteAnimationData::IsLoop() const
	{
		return m_isLoop;
	}

    const std::string& SpriteAnimationData::GetName() const
    {
        return m_name;
    }

	const std::vector<SpriteFrameInfo>& SpriteAnimationData::GetFrames() const
	{
		return m_frames;
	}

	const std::vector<SpriteEventInfo>& SpriteAnimationData::GetEvents() const
	{
		return m_events;
	}

    void SpriteAnimationData::SetupFramesIndex(const SpriteData* spriteData)
    {
        for (auto& frame : m_frames)
        {
            frame.pieceIndex = spriteData->GetSpriteIndex(frame.pieceName);
        }
    }
}