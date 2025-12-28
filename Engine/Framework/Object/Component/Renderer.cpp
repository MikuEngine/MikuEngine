#include "pch.h"
#include "Renderer.h"

#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"

namespace engine
{
	Renderer::Renderer()
	{
		m_systemIndices.fill(-1);

		SystemManager::Get().GetRenderSystem().Register(this);
	}

	Renderer::~Renderer()
	{
		SystemManager::Get().GetRenderSystem().Unregister(this);
	}
}