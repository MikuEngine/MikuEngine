#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
	enum class RenderType
	{
		Opaque,
		Cutout,
		Transparent,
		Screen,
		Shadow,
	};

	class Renderer :
		public Component
	{
		
	};
}