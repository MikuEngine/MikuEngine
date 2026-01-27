#include "EnginePCH.h"
#include "Renderer.h"

#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/Object/Component/Transform.h"

namespace engine
{
	Renderer::Renderer()
	{
		m_systemIndices.fill(-1);
	}

	Renderer::~Renderer()
	{
		SystemManager::Get().GetRenderSystem().Unregister(this);
	}

	void Renderer::Initialize()
	{
		SystemManager::Get().GetRenderSystem().Register(this);
	}

	Matrix Renderer::GetSocketWorldMatrix(const std::string& name) const
	{
		for (const auto& instance : m_socketInstances)
		{
			if (instance.info->name == name)
			{
				return instance.worldMatrix;
			}
		}

		return GetTransform()->GetWorld();
	}
}