#pragma once

#include "Framework/System/System.h"
#include "Framework/Object/Component/Light.h"

namespace engine
{
	class LightSystem :
		public System<Light>
	{
	public:
		Light* GetMainLight() const;
		const std::vector<Light*>& GetLights() const;
	};
}