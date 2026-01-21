#pragma once
#include "AssetData.h"
#include "Framework/System/SoundSystem.h"

namespace engine
{
	class SoundData : public AssetData
	{
	private:
		Sound* m_sound = nullptr;

	public:
		SoundData(Sound* sound)
			: m_sound(sound)
		{

		}

		virtual ~SoundData()
		{
			if (m_sound)
			{
				m_sound->Release();
				delete m_sound;
				m_sound = nullptr;
			}
		}

		Sound* GetSound() const { return m_sound; }
	};
}