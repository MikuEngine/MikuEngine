#include "pch.h"
#include "GraphicsDevice.h"

#include <dxgi1_5.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include "Common/Utility/Profiling.h"

namespace engine
{
    struct DeviceResources
    {
        Microsoft::WRL::ComPtr<ID3D11Device> device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;

        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> gameRTV;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> gameDSV;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> gameSRV;

        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;

        Microsoft::WRL::ComPtr<IDXGIAdapter3> dxgiAdapter;

        // Resources for Blit
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> defaultVS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> defaultPS;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> defaultInputLayout;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> defaultSamplerState;
        UINT vertexCount = 0;
        UINT indexCount = 0;
        UINT stride = 0;
        UINT offset = 0;
    };

    GraphicsDevice::GraphicsDevice()
        : m_resource{ std::make_unique<DeviceResources>() }
    {
    }

    GraphicsDevice::~GraphicsDevice() = default;

    void GraphicsDevice::Initialize(
        HWND hWnd,
        UINT resolutionWidth,
        UINT resolutionHeight,
        UINT screenWidth,
        UINT screenHeight,
        bool isFullScreen,
        bool useVsync)
    {
        m_hWnd = hWnd;
        m_resolutionWidth = resolutionWidth;
        m_resolutionHeight = resolutionHeight;
        m_screenWidth = screenWidth;
        m_screenHeight = screenHeight;
        m_isFullScreen = isFullScreen;

        // create device, device context, swap chain
        {
            UINT dxgiFactoryCreationFlags = 0;
#ifdef _DEBUG
            dxgiFactoryCreationFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG

            Microsoft::WRL::ComPtr<IDXGIFactory5> dxgiFactory;
            HR_CHECK(CreateDXGIFactory2(dxgiFactoryCreationFlags, IID_PPV_ARGS(&dxgiFactory)));

            // create device, device context
            {
                Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter = GetHighPerformanceAdapter(dxgiFactory);

                UINT d3dCreationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
                d3dCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

                const D3D_FEATURE_LEVEL featureLevels[]{
                    D3D_FEATURE_LEVEL_12_2,
                    D3D_FEATURE_LEVEL_12_1,
                    D3D_FEATURE_LEVEL_12_0,
                    D3D_FEATURE_LEVEL_11_1,
                    D3D_FEATURE_LEVEL_11_0
                };

                D3D_FEATURE_LEVEL actualFeatureLevel;

                HR_CHECK(D3D11CreateDevice(
                    adapter.Get(),
                    D3D_DRIVER_TYPE_UNKNOWN,
                    nullptr,
                    d3dCreationFlags,
                    featureLevels,
                    ARRAYSIZE(featureLevels),
                    D3D11_SDK_VERSION,
                    &m_resource->device,
                    &actualFeatureLevel,
                    &m_resource->deviceContext));
            }

            // create swap chain
            {
                BOOL allowTearing = FALSE;
                HR_CHECK(dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)));

                if (allowTearing)
                {
                    m_tearingSupport = true;
                }

                DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
                swapChainDesc.Width = m_isFullScreen ? m_screenWidth : m_resolutionWidth;
                swapChainDesc.Height = m_isFullScreen ? m_screenHeight : m_resolutionHeight;
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

                SetVsync(useVsync);

                HR_CHECK(dxgiFactory->CreateSwapChainForHwnd(
                    m_resource->device.Get(),
                    m_hWnd,
                    &swapChainDesc,
                    nullptr,
                    nullptr,
                    &m_resource->swapChain));
            }
        }

        // directx alt+enter 전체화면 전환 해제
        if (Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
            SUCCEEDED(m_resource->swapChain->GetParent(__uuidof (IDXGIFactory1), &factory)))
        {
            factory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
        }

        CreateSizeDependentResources();

        Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
        Microsoft::WRL::ComPtr<IDXGIDevice3> dxgiDevice;
        m_resource->device.As(&dxgiDevice);
        dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());
        dxgiAdapter.As(&m_resource->dxgiAdapter);

        // Create Fullscreen Quad Mesh
        {
            MeshData quadData = GeometryGenerator::CreateFullscreenQuad();
            m_resource->vertexCount = static_cast<UINT>(quadData.vertices.size());
            m_resource->indexCount = static_cast<UINT>(quadData.indices.size());
            m_resource->stride = sizeof(CommonVertex);
            m_resource->offset = 0;

            D3D11_BUFFER_DESC vertexBufferDesc{};
            vertexBufferDesc.ByteWidth = sizeof(CommonVertex) * m_resource->vertexCount;
            vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
            vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vertexBufferDesc.CPUAccessFlags = 0;
            vertexBufferDesc.MiscFlags = 0;
            vertexBufferDesc.StructureByteStride = 0;

            D3D11_SUBRESOURCE_DATA vertexData{};
            vertexData.pSysMem = quadData.vertices.data();
            vertexData.SysMemPitch = 0;
            vertexData.SysMemSlicePitch = 0;

            HR_CHECK(GraphicsDevice::Get().GetDevice()->CreateBuffer(&vertexBufferDesc, &vertexData, &m_resource->vertexBuffer));

            D3D11_BUFFER_DESC indexBufferDesc{};
            indexBufferDesc.ByteWidth = sizeof(uint32_t) * m_resource->indexCount;
            indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
            indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            indexBufferDesc.CPUAccessFlags = 0;
            indexBufferDesc.MiscFlags = 0;
            indexBufferDesc.StructureByteStride = 0;

            D3D11_SUBRESOURCE_DATA indexData{};
            indexData.pSysMem = quadData.indices.data();
            indexData.SysMemPitch = 0;
            indexData.SysMemSlicePitch = 0;

            HR_CHECK(GraphicsDevice::Get().GetDevice()->CreateBuffer(&indexBufferDesc, &indexData, &m_resource->indexBuffer));
        }

        // Create Default Shaders & Layout
        {
            Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
            CompileShaderFromFile("Shader/Vertex/DefaultVertexShader.hlsl", "main", "vs_5_0", vsBlob);
            HR_CHECK(m_resource->device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_resource->defaultVS));

            D3D11_INPUT_ELEMENT_DESC layout[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            HR_CHECK(m_resource->device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_resource->defaultInputLayout));

            Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
            CompileShaderFromFile("Shader/Pixel/DefaultPixelShader.hlsl", "main", "ps_5_0", psBlob);
            HR_CHECK(m_resource->device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_resource->defaultPS));
        }

        // Create Sampler State
        {
            D3D11_SAMPLER_DESC samplerDesc{};
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            samplerDesc.MinLOD = 0;
            samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

            HR_CHECK(m_resource->device->CreateSamplerState(&samplerDesc, &m_resource->defaultSamplerState));
        }
    }

    void GraphicsDevice::Shutdown()
    {
        m_resource.reset();
    }

    bool GraphicsDevice::Resize(
        UINT resolutionWidth,
        UINT resolutionHeight,
        UINT screenWidth,
        UINT screenHeight,
        bool isFullScreen)
    {
        if (m_resolutionWidth == resolutionWidth &&
            m_resolutionHeight == resolutionHeight &&
            m_isFullScreen == isFullScreen)
        {
            return false;
        }

        if (resolutionWidth == 0 || resolutionHeight == 0)
        {
            return false;
        }

        m_resolutionWidth = resolutionWidth;
        m_resolutionHeight = resolutionHeight;
        m_isFullScreen = isFullScreen;

        m_resource->deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
        m_resource->backBufferRTV.Reset();
        m_resource->gameRTV.Reset();
        m_resource->gameDSV.Reset();
        m_resource->gameSRV.Reset();

        UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        if (m_tearingSupport)
        {
            swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }
        HR_CHECK(m_resource->swapChain->ResizeBuffers(
            0,
            m_isFullScreen ? m_screenWidth : m_resolutionWidth,
            m_isFullScreen ? m_screenHeight : m_resolutionHeight,
            DXGI_FORMAT_UNKNOWN,
            swapChainFlags));

        CreateSizeDependentResources();

        return true;
    }

    void GraphicsDevice::BeginDraw(const Color& clearColor)
    {
        m_resource->deviceContext->OMSetRenderTargets(1, m_resource->gameRTV.GetAddressOf(), m_resource->gameDSV.Get());

        m_resource->deviceContext->ClearRenderTargetView(m_resource->gameRTV.Get(), clearColor);
        m_resource->deviceContext->ClearDepthStencilView(m_resource->gameDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        m_resource->deviceContext->RSSetViewports(1, &m_gameViewport);
    }

    void GraphicsDevice::BackBufferDraw()
    {
        m_resource->deviceContext->OMSetRenderTargets(1, m_resource->backBufferRTV.GetAddressOf(), nullptr);

        m_resource->deviceContext->ClearRenderTargetView(m_resource->backBufferRTV.Get(), Color(0.0f, 0.0f, 0.0f, 1.0f));

        m_resource->deviceContext->RSSetViewports(1, &m_backBufferViewport);

        // Blit
        {
            // Input Assembler
            m_resource->deviceContext->IASetInputLayout(m_resource->defaultInputLayout.Get());
            m_resource->deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // Shaders
            m_resource->deviceContext->VSSetShader(m_resource->defaultVS.Get(), nullptr, 0);
            m_resource->deviceContext->PSSetShader(m_resource->defaultPS.Get(), nullptr, 0);

            // Resources
            m_resource->deviceContext->PSSetShaderResources(0, 1, m_resource->gameSRV.GetAddressOf());
            m_resource->deviceContext->PSSetSamplers(0, 1, m_resource->defaultSamplerState.GetAddressOf());

            // Draw
            m_resource->deviceContext->IASetVertexBuffers(0, 1, m_resource->vertexBuffer.GetAddressOf(), &m_resource->stride, &m_resource->offset);
            m_resource->deviceContext->IASetIndexBuffer(m_resource->indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
            m_resource->deviceContext->DrawIndexed(m_resource->indexCount, 0, 0);

            // Unbind
            ID3D11ShaderResourceView* nullSRV = nullptr;
            m_resource->deviceContext->PSSetShaderResources(0, 1, &nullSRV);
        }
    }

    void GraphicsDevice::EndDraw()
    {
        m_resource->swapChain->Present(m_syncInterval, m_presentFlags);

        // vram usage
        {
            DXGI_QUERY_VIDEO_MEMORY_INFO memInfo{};
            m_resource->dxgiAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memInfo);
            Profiling::UpdateVRAMUsage(memInfo.CurrentUsage);
        }
    }

    const Microsoft::WRL::ComPtr<ID3D11Device>& GraphicsDevice::GetDevice() const
    {
        return m_resource->device;
    }

    const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& GraphicsDevice::GetDeviceContext() const
    {
        return m_resource->deviceContext;
    }

    const Microsoft::WRL::ComPtr<IDXGISwapChain1>& GraphicsDevice::GetSwapChain() const
    {
        return m_resource->swapChain;
    }

    const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& GraphicsDevice::GetRenderTargetView() const
    {
        return m_resource->gameRTV;
    }

    const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& GraphicsDevice::GetDepthStencilView() const
    {
        return m_resource->gameDSV;
    }

    const D3D11_VIEWPORT& GraphicsDevice::GetViewport() const
    {
        return m_gameViewport;
    }

    void GraphicsDevice::SetVsync(bool useVsync)
    {
        m_useVsync = useVsync;
        if (m_useVsync || !m_tearingSupport)
        {
            m_syncInterval = 1;
            m_presentFlags = 0;
        }
        else if (m_tearingSupport)
        {
            m_syncInterval = 0;
            m_presentFlags = DXGI_PRESENT_ALLOW_TEARING;
        }
    }

    void GraphicsDevice::CompileShaderFromFile(
        const std::filesystem::path& fileName,
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
            (fileName).c_str(),
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
        HR_CHECK(m_resource->swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
        HR_CHECK(m_resource->device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_resource->backBufferRTV));

        D3D11_TEXTURE2D_DESC depthStencilDesc{};
        depthStencilDesc.Width = m_resolutionWidth;
        depthStencilDesc.Height = m_resolutionHeight;
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
        HR_CHECK(m_resource->device->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencilTexture));

        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
        depthStencilViewDesc.Format = depthStencilDesc.Format;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDesc.Texture2D.MipSlice = 0;

        HR_CHECK(m_resource->device->CreateDepthStencilView(depthStencilTexture.Get(), &depthStencilViewDesc, &m_resource->gameDSV));

        // 1. Create Off-screen Texture
        D3D11_TEXTURE2D_DESC gameTextureDesc{};
        gameTextureDesc.Width = m_resolutionWidth;
        gameTextureDesc.Height = m_resolutionHeight;
        gameTextureDesc.MipLevels = 1;
        gameTextureDesc.ArraySize = 1;
        gameTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        gameTextureDesc.SampleDesc.Count = 1;
        gameTextureDesc.Usage = D3D11_USAGE_DEFAULT;
        gameTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        gameTextureDesc.CPUAccessFlags = 0;
        gameTextureDesc.MiscFlags = 0;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> gameTexture;
        HR_CHECK(m_resource->device->CreateTexture2D(&gameTextureDesc, nullptr, &gameTexture));

        // 2. Create RTV for Game
        HR_CHECK(m_resource->device->CreateRenderTargetView(gameTexture.Get(), nullptr, &m_resource->gameRTV));

        // 3. Create SRV for Game (to be sampled in EndDraw)
        HR_CHECK(m_resource->device->CreateShaderResourceView(gameTexture.Get(), nullptr, &m_resource->gameSRV));

        m_gameViewport.TopLeftX = 0.0f;
        m_gameViewport.TopLeftY = 0.0f;
        m_gameViewport.Width = static_cast<float>(m_resolutionWidth);
        m_gameViewport.Height = static_cast<float>(m_resolutionHeight);
        m_gameViewport.MinDepth = 0.0f;
        m_gameViewport.MaxDepth = 1.0f;

        m_backBufferViewport.TopLeftX = 0.0f;
        m_backBufferViewport.TopLeftY = 0.0f;
        m_backBufferViewport.MinDepth = 0.0f;
        m_backBufferViewport.MaxDepth = 1.0f;

        float targetAspectRatio = static_cast<float>(m_resolutionWidth) / static_cast<float>(m_resolutionHeight);
        float screenAspectRatio = static_cast<float>(m_screenWidth) / static_cast<float>(m_screenHeight);

        if (m_isFullScreen)
        {
            float viewWidth = static_cast<float>(m_screenWidth);
            float viewHeight = static_cast<float>(m_screenHeight);
            float viewX = 0.0f;
            float viewY = 0.0f;

            if (screenAspectRatio > targetAspectRatio)
            {
                viewWidth = m_screenHeight * targetAspectRatio;
                viewX = (m_screenWidth - viewWidth) * 0.5f;
            }
            else
            {
                viewHeight = m_screenWidth / targetAspectRatio;
                viewY = (m_screenHeight - viewHeight) * 0.5f;
            }

            m_backBufferViewport.TopLeftX = viewX;
            m_backBufferViewport.TopLeftY = viewY;
            m_backBufferViewport.Width = viewWidth;
            m_backBufferViewport.Height = viewHeight;
        }
        else
        {
            m_backBufferViewport.Width = static_cast<float>(m_resolutionWidth);
            m_backBufferViewport.Height = static_cast<float>(m_resolutionHeight);
        }
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter1> GraphicsDevice::GetHighPerformanceAdapter(Microsoft::WRL::ComPtr<IDXGIFactory5> factory)
    {
        Microsoft::WRL::ComPtr<IDXGIFactory6> factory6;
        HRESULT hr = factory.As(&factory6);

        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

        if (SUCCEEDED(hr))
        {
            for (UINT i = 0;
                factory6->EnumAdapterByGpuPreference(
                    i,
                    DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, // 핵심: 고성능 GPU 요청
                    IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
                    ++i)
            {
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    continue;
                }

                if (SUCCEEDED(D3D11CreateDevice(
                    adapter.Get(),
                    D3D_DRIVER_TYPE_UNKNOWN,
                    nullptr,
                    0,
                    nullptr,
                    0,
                    D3D11_SDK_VERSION,
                    nullptr, nullptr, nullptr)))
                {
                    break;
                }
            }
        }

        if (adapter == nullptr)
        {
            for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
            {
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    continue;
                }

                if (SUCCEEDED(D3D11CreateDevice(
                    adapter.Get(),
                    D3D_DRIVER_TYPE_UNKNOWN,
                    nullptr,
                    0,
                    nullptr,
                    0,
                    D3D11_SDK_VERSION,
                    nullptr, nullptr, nullptr)))
                {
                    break;
                }
            }
        }

        return adapter;
    }
}
