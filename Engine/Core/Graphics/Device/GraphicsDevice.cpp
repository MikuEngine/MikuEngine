#include "pch.h"
#include "GraphicsDevice.h"

#include <dxgi1_5.h>
#include <d3dcompiler.h>

#include "Common/Utility/Profiling.h"

namespace engine
{
	void GraphicsDevice::Initialize(HWND hWnd, UINT width, UINT height, bool useVsync)
	{
		m_hWnd = hWnd;
		m_width = width;
		m_height = height;

		// create device, device context
		{
			UINT d3dCreationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
			d3dCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DBUG

			const D3D_FEATURE_LEVEL featureLevels[]{
				D3D_FEATURE_LEVEL_12_2,
				D3D_FEATURE_LEVEL_12_1,
				D3D_FEATURE_LEVEL_12_0,
				D3D_FEATURE_LEVEL_11_1,
				D3D_FEATURE_LEVEL_11_0
			};

			D3D_FEATURE_LEVEL actualFeatureLevel;

			HR_CHECK(D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_HARDWARE,
				nullptr,
				d3dCreationFlags,
				featureLevels,
				ARRAYSIZE(featureLevels),
				D3D11_SDK_VERSION,
				&m_d3d11Device,
				&actualFeatureLevel,
				&m_d3d11DeviceContext));
		}

		// swap chain
		{
			UINT dxgiFactoryCreationFlags = 0;
#ifdef _DEBUG
			dxgiFactoryCreationFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG

			Microsoft::WRL::ComPtr<IDXGIFactory5> dxgiFactory;
			HR_CHECK(CreateDXGIFactory2(dxgiFactoryCreationFlags, IID_PPV_ARGS(&dxgiFactory)));

			BOOL allowTearing = FALSE;
			HR_CHECK(dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)));

			if (allowTearing)
			{
				m_tearingSupport = true;
			}

			SetVsync(useVsync);

			DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
			swapChainDesc.Width = m_width;
			swapChainDesc.Height = m_height;
			swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferCount = 2;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			swapChainDesc.Scaling = DXGI_SCALING_NONE;
			swapChainDesc.Stereo = FALSE;
			swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			if (m_tearingSupport)
			{
				swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
			}

			HR_CHECK(dxgiFactory->CreateSwapChainForHwnd(
				m_d3d11Device.Get(),
				m_hWnd,
				&swapChainDesc,
				nullptr,
				nullptr,
				&m_dxgiSwapChain));
		}

		CreateSizeDependentResources();

		Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
		Microsoft::WRL::ComPtr<IDXGIDevice3> dxgiDevice;
		m_d3d11Device.As(&dxgiDevice);
		dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());
		dxgiAdapter.As(&m_dxgiAdapter);
	}

	bool GraphicsDevice::Resize(UINT width, UINT height)
	{
		if (m_width == width && m_height == height)
		{
			return false;
		}

		if (width == 0 || height == 0)
		{
			return false;
		}

		m_width = width;
		m_height = height;

		m_d3d11DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
		m_d3d11RenderTargetView.Reset();
		m_d3d11DepthStencilView.Reset();

		HR_CHECK(m_dxgiSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

		CreateSizeDependentResources();

		return true;
	}

	void GraphicsDevice::BeginDraw(const Color& clearColor)
	{
		m_d3d11DeviceContext->OMSetRenderTargets(1, m_d3d11RenderTargetView.GetAddressOf(), m_d3d11DepthStencilView.Get());

		m_d3d11DeviceContext->ClearRenderTargetView(m_d3d11RenderTargetView.Get(), clearColor);
		m_d3d11DeviceContext->ClearDepthStencilView(m_d3d11DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		m_d3d11DeviceContext->RSSetViewports(1, &m_viewport);
	}

	void GraphicsDevice::EndDraw()
	{
		m_dxgiSwapChain->Present(m_syncInterval, m_presentFlags);

		// vram usage
		{
			DXGI_QUERY_VIDEO_MEMORY_INFO memInfo{};
			m_dxgiAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memInfo);
			Profiling::UpdateVRAMUsage(memInfo.CurrentUsage);
		}
	}

	const Microsoft::WRL::ComPtr<ID3D11Device>& GraphicsDevice::GetDevice() const
	{
		return m_d3d11Device;
	}

	const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& GraphicsDevice::GetDeviceContext() const
	{
		return m_d3d11DeviceContext;
	}

	const Microsoft::WRL::ComPtr<IDXGISwapChain1>& GraphicsDevice::GetSwapChain() const
	{
		return m_dxgiSwapChain;
	}

	const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& GraphicsDevice::GetRenderTargetView() const
	{
		return m_d3d11RenderTargetView;
	}

	const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& GraphicsDevice::GetDepthStencilView() const
	{
		return m_d3d11DepthStencilView;
	}

	const D3D11_VIEWPORT& GraphicsDevice::GetViewport() const
	{
		return m_viewport;
	}

	void GraphicsDevice::SetVsync(bool useVsync)
	{
		m_useVsync = useVsync;
		if (m_useVsync)
		{
			m_syncInterval = 1;
			m_presentFlags = 0;
		}
		else
		{
			m_syncInterval = 0;
			m_presentFlags = DXGI_PRESENT_ALLOW_TEARING;
		}
	}

	void GraphicsDevice::CompileShaderFromFile(
		const std::string& fileName,
		const std::string& entryPoint,
		const std::string& shaderModel,
		Microsoft::WRL::ComPtr<ID3DBlob>& blobOut)
	{
		DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
		shaderFlags |= D3DCOMPILE_DEBUG;
		shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG

		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

		HR_CHECK(D3DCompileFromFile(
			ToWideChar(fileName).c_str(),
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entryPoint.c_str(),
			shaderModel.c_str(),
			shaderFlags,
			0,
			&blobOut,
			&errorBlob));
	}

	void GraphicsDevice::CreateSizeDependentResources()
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
		HR_CHECK(m_dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
		HR_CHECK(m_d3d11Device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_d3d11RenderTargetView));

		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = m_width;
		depthStencilDesc.Height = m_height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilTexture;
		HR_CHECK(m_d3d11Device->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencilTexture));

		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = depthStencilDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		HR_CHECK(m_d3d11Device->CreateDepthStencilView(depthStencilTexture.Get(), &depthStencilViewDesc, &m_d3d11DepthStencilView));

		
		m_viewport.TopLeftX = 0.0f;
		m_viewport.TopLeftY = 0.0f;
		m_viewport.Width = static_cast<float>(m_width);
		m_viewport.Height = static_cast<float>(m_height);
		m_viewport.MinDepth = 0.0f;
		m_viewport.MaxDepth = 1.0f;

		m_d3d11DeviceContext->RSSetViewports(1, &m_viewport);
	}
}
