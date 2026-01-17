#include "EnginePCH.h"
#include "EditorGrid.h"

#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Resource/VertexBuffer.h"
#include "Core/Graphics/Resource/IndexBuffer.h"
#include "Core/Graphics/Resource/VertexShader.h"
#include "Core/Graphics/Resource/PixelShader.h"
#include "Core/Graphics/Resource/InputLayout.h"
#include "Core/Graphics/Resource/BlendState.h"
#include "Core/Graphics/Resource/SamplerState.h"
#include "Core/Graphics/Resource/DepthStencilState.h"
#include "Core/Graphics/Resource/ConstantBuffer.h"
#include "Core/Graphics/Resource/RasterizerState.h"
#include "Core/Graphics/Data/ConstantBufferTypes.h"
#include "Core/Graphics/Device/GraphicsDevice.h"
#include "Editor/EditorManager.h"
#include "Editor/EditorCamera.h"

namespace engine
{
	void EditorGrid::Initialize()
	{
        m_gridPlaneVB = ResourceManager::Get().GetGeometryVertexBuffer("DefaultPlane");
        m_gridPlaneIB = ResourceManager::Get().GetGeometryIndexBuffer("DefaultPlane");

		m_gridVS = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Grid_VS.hlsl");
		m_gridPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Grid_PS.hlsl");

		m_gridInputLayout = m_gridVS->GetOrCreateInputLayout<PositionVertex>();
		m_dss = ResourceManager::Get().GetDefaultDepthStencilState(DefaultDepthStencilType::DepthRead);
		m_samplerLinear = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);
		m_gridCB = ResourceManager::Get().GetOrCreateConstantBuffer("Grid", sizeof(CbGrid));
		m_objectCB = ResourceManager::Get().GetOrCreateConstantBuffer("Object", sizeof(CbObject));

		{
			D3D11_RASTERIZER_DESC desc{};
			desc.FillMode = D3D11_FILL_SOLID;
			desc.CullMode = D3D11_CULL_NONE;
			desc.FrontCounterClockwise = false;
			desc.DepthBias = -10;
			desc.DepthBiasClamp = 0.0f;
			desc.SlopeScaledDepthBias = -1.0f;
			desc.DepthClipEnable = true;

			m_rss = ResourceManager::Get().GetOrCreateRasterizerState("Grid", desc);
		}

		m_bs = ResourceManager::Get().GetDefaultBlendState(DefaultBlendType::AlphaBlend);
	}

	void EditorGrid::Draw() const
	{
		const auto& graphics = GraphicsDevice::Get();
		const auto& context = graphics.GetDeviceContext();

		static const UINT stride = m_gridPlaneVB->GetBufferStride();
		static constexpr UINT offset = 0;
		static constexpr float factor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };

		auto camera = EditorManager::Get().GetEditorCamera();
		const auto& cameraPosition = camera->GetPosition();
		const float farDist = camera->GetFar();

		const auto world = Matrix::CreateScale(farDist, 1.0f, farDist) *
			Matrix::CreateTranslation(Vector3(cameraPosition.x, 0.0f, cameraPosition.z));

		CbObject cbObject{};
		cbObject.world = world.Transpose();

		CbGrid cbGrid{};
		cbGrid.gridColor = Vector4(0.5f, 0.5f, 0.5f, 1.0f);
		cbGrid.gridSpacing = 1.0f;
		cbGrid.gridWidth = 1.0f;

		context->IASetInputLayout(m_gridInputLayout->GetRawInputLayout());
		context->IASetVertexBuffers(0, 1, m_gridPlaneVB->GetBuffer().GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(m_gridPlaneIB->GetRawBuffer(), m_gridPlaneIB->GetIndexFormat(), 0);
		context->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerLinear->GetSamplerState().GetAddressOf());
		context->VSSetShader(m_gridVS->GetRawShader(), nullptr, 0);
		context->RSSetState(m_rss->GetRawRasterizerState());
		context->PSSetShader(m_gridPS->GetRawShader(), nullptr, 0);
		context->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Object), 1, m_objectCB->GetBuffer().GetAddressOf());
		context->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Object), 1, m_objectCB->GetBuffer().GetAddressOf());
		context->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Grid), 1, m_gridCB->GetBuffer().GetAddressOf());
		context->UpdateSubresource(m_objectCB->GetRawBuffer(), 0, nullptr, &cbObject, 0, 0);
		context->UpdateSubresource(m_gridCB->GetRawBuffer(), 0, nullptr, &cbGrid, 0, 0);
		context->OMSetDepthStencilState(m_dss->GetRawDepthStencilState(), 0);
		context->OMSetBlendState(m_bs->GetRawBlendState(), factor, 0xffffffff);
		context->DrawIndexed(m_gridPlaneIB->GetIndexCount(), 0, 0);

		context->OMSetDepthStencilState(nullptr, 0);
		context->OMSetBlendState(nullptr, factor, 0xffffffff);
		context->RSSetState(nullptr);
	}
}