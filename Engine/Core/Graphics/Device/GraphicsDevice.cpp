#include "EnginePCH.h"
#include "GraphicsDevice.h"

#include <dxgi1_5.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include "Common/Utility/Profiling.h"
#include "Core/Graphics/Data/Vertex.h"
#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Resource/Texture.h"
#include "Core/Graphics/Resource/RasterizerState.h"
#include "Core/Graphics/Resource/VertexBuffer.h"
#include "Core/Graphics/Resource/IndexBuffer.h"
#include "Core/Graphics/Resource/VertexShader.h"
#include "Core/Graphics/Resource/PixelShader.h"
#include "Core/Graphics/Resource/InputLayout.h"
#include "Core/Graphics/Resource/SamplerState.h"
#include "Core/Graphics/Resource/ConstantBuffer.h"
#include "Core/Graphics/Resource/DepthStencilState.h"
#include "Core/Graphics/Data/ConstantBufferTypes.h"
#include "Core/Graphics/Data/ShaderSlotTypes.h"
#include "Framework/Asset/GeometryGenerator.h"

namespace engine
{
    void GBufferResources::Reset()
    {
        baseColor.reset();
        normal.reset();
        orm.reset();
        emissive.reset();
    }

    std::array<ID3D11RenderTargetView*, GBufferResources::count> GBufferResources::GetRawRTVs() const
    {
        return {
            baseColor->GetRawRTV(),
            normal->GetRawRTV(),
            orm->GetRawRTV(),
            emissive->GetRawRTV()
        };
    }

    std::array<ID3D11ShaderResourceView*, GBufferResources::count> GBufferResources::GetRawSRVs() const
    {
        return {
            baseColor->GetRawSRV(),
            normal->GetRawSRV(),
            orm->GetRawSRV(),
            emissive->GetRawSRV()
        };
    }

    GraphicsDevice::GraphicsDevice() = default;
    GraphicsDevice::~GraphicsDevice() = default;

    void GraphicsDevice::Initialize(
        HWND hWnd,
        UINT resolutionWidth,
        UINT resolutionHeight,
        UINT screenWidth,
        UINT screenHeight,
        bool isFullscreen,
        bool useVsync)
    {
        m_hWnd = hWnd;
        m_resolutionWidth = resolutionWidth;
        m_resolutionHeight = resolutionHeight;
        m_screenWidth = screenWidth;
        m_screenHeight = screenHeight;
        m_isFullscreen = isFullscreen;

        CheckHDRSupportAndGetMaxNits();
        CreateCoreResources();
        
        // GraphicsDevice::Get().GetDevice 등 사용 가능
        SetVsync(useVsync);
    }

    void GraphicsDevice::InitializeResources()
    {
        CreateSizeDependentResources();

        Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
        Microsoft::WRL::ComPtr<IDXGIDevice3> dxgiDevice;
        m_device.As(&dxgiDevice);
        dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());
        dxgiAdapter.As(&m_dxgiAdapter);

        CreateAdditionalResources();
        CreateShadowBuffer();
    }

    void GraphicsDevice::Shutdown()
    {
        m_quadIndexBuffer.reset();
        m_quadVertexBuffer.reset();

        m_samplerLinear.reset();
        m_samplerPoint.reset();
        m_quadInputLayout.reset();

        m_ldrPS.reset();
        m_hdrPS.reset();
        m_brightPassPS.reset();
        m_blurPS.reset();
        m_fxaaPS.reset();
        m_globalLightPS.reset();
        m_blitPS.reset();
        m_outlinePS.reset();
        m_fullscreenQuadVS.reset();

        m_dxgiAdapter.Reset();
        m_blurConstantBuffer.reset();
        m_screenSizeCB.reset();

        m_maskDSS.reset();

        m_gBuffer.Reset();

        m_shadowMapRSS.reset();
        m_shadowDepthBuffer.reset();
        m_gameDepthBuffer.reset();
        m_hdrBuffer.reset();
        m_finalBuffer.reset();
        m_bloomHalfBuffer.reset();
        m_bloomWorkBuffer.reset();
        m_aaBuffer.reset();
        m_outlineStencilBuffer.reset();

        m_backBufferRTV.Reset();

        m_swapChain.Reset();
        m_deviceContext.Reset();
        m_device.Reset();
    }

    bool GraphicsDevice::Resize(
        UINT resolutionWidth,
        UINT resolutionHeight,
        UINT screenWidth,
        UINT screenHeight,
        bool isFullscreen)
    {
        if (m_resolutionWidth == resolutionWidth &&
            m_resolutionHeight == resolutionHeight &&
            m_isFullscreen == isFullscreen)
        {
            return false;
        }

        if (resolutionWidth == 0 || resolutionHeight == 0)
        {
            return false;
        }

        m_resolutionWidth = resolutionWidth;
        m_resolutionHeight = resolutionHeight;
        m_isFullscreen = isFullscreen;

        m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);

        UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        if (m_tearingSupport)
        {
            swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        HR_CHECK(m_swapChain->ResizeBuffers(
            0,
            m_isFullscreen ? m_screenWidth : m_resolutionWidth,
            m_isFullscreen ? m_screenHeight : m_resolutionHeight,
            DXGI_FORMAT_UNKNOWN,
            swapChainFlags));

        CreateSizeDependentResources();

        return true;
    }

    void GraphicsDevice::DrawFullscreenQuad()
    {
        m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_deviceContext->IASetInputLayout(m_quadInputLayout->GetRawInputLayout());
        static const UINT offset = 0;
        static const UINT stride = m_quadVertexBuffer->GetBufferStride();
        m_deviceContext->IASetVertexBuffers(0, 1, m_quadVertexBuffer->GetBuffer().GetAddressOf(), &stride, &offset);
        m_deviceContext->IASetIndexBuffer(m_quadIndexBuffer->GetRawBuffer(), m_quadIndexBuffer->GetIndexFormat(), 0);
        
        m_deviceContext->VSSetShader(m_fullscreenQuadVS->GetRawShader(), nullptr, 0);

        m_deviceContext->DrawIndexed(m_quadIndexBuffer->GetIndexCount(), 0, 0);
    }

    void GraphicsDevice::ClearAllViews()
    {
        constexpr float clearColor[4]{ 0.0f, 0.0f, 0.0f, 1.0f };

        m_deviceContext->ClearDepthStencilView(m_shadowDepthBuffer->GetRawDSV(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        m_deviceContext->ClearDepthStencilView(m_gameDepthBuffer->GetRawDSV(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        m_deviceContext->ClearDepthStencilView(m_outlineStencilBuffer->GetRawDSV(), D3D11_CLEAR_STENCIL, 0.0f, 0);
        m_deviceContext->ClearRenderTargetView(m_gBuffer.baseColor->GetRawRTV(), clearColor);
        m_deviceContext->ClearRenderTargetView(m_gBuffer.normal->GetRawRTV(), clearColor);
        m_deviceContext->ClearRenderTargetView(m_gBuffer.orm->GetRawRTV(), clearColor);
        m_deviceContext->ClearRenderTargetView(m_gBuffer.emissive->GetRawRTV(), clearColor);
        m_deviceContext->ClearRenderTargetView(m_hdrBuffer->GetRawRTV(), clearColor);
        m_deviceContext->ClearRenderTargetView(m_finalBuffer->GetRawRTV(), clearColor);
        m_deviceContext->ClearRenderTargetView(m_backBufferRTV.Get(), clearColor);
        m_deviceContext->ClearRenderTargetView(m_bloomHalfBuffer->GetRawRTV(), clearColor);
        m_deviceContext->ClearRenderTargetView(m_bloomWorkBuffer->GetRawRTV(), clearColor);
        m_deviceContext->ClearRenderTargetView(m_aaBuffer->GetRawRTV(), clearColor);
    }

    void GraphicsDevice::BeginDrawShadowPass()
    {
        m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_deviceContext->RSSetViewports(1, &m_shadowViewport);
        m_deviceContext->RSSetState(m_shadowMapRSS->GetRawRasterizerState());

        m_deviceContext->OMSetRenderTargets(0, nullptr, m_shadowDepthBuffer->GetRawDSV());
        m_deviceContext->OMSetDepthStencilState(nullptr, 0);
    }

    void GraphicsDevice::EndDrawShadowPass()
    {
        m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    }

    void GraphicsDevice::BeginDrawGeometryPass()
    {
        m_deviceContext->RSSetViewports(1, &m_gameViewport);
        m_deviceContext->RSSetState(nullptr);

        m_deviceContext->OMSetRenderTargets(m_gBuffer.count, m_gBuffer.GetRawRTVs().data(), m_gameDepthBuffer->GetRawDSV());
        m_deviceContext->OMSetDepthStencilState(nullptr, 0);

        constexpr float blendFactor[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
        m_deviceContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
    }

    void GraphicsDevice::EndDrawGeometryPass()
    {
        m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    }

    void GraphicsDevice::BeginDrawLightPass()
    {
        m_deviceContext->RSSetState(nullptr);

        m_deviceContext->PSSetShader(m_globalLightPS->GetRawShader(), nullptr, 0);
        m_deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerLinear->GetSamplerState().GetAddressOf());
        m_deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Point), 1, m_samplerPoint->GetSamplerState().GetAddressOf());

        m_deviceContext->PSSetShaderResources(m_gBuffer.startSlot, m_gBuffer.count, m_gBuffer.GetRawSRVs().data());
        m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::GBufferDepth), 1, m_gameDepthBuffer->GetSRV().GetAddressOf());
        m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::ShadowMap), 1, m_shadowDepthBuffer->GetSRV().GetAddressOf());

        m_deviceContext->OMSetRenderTargets(1, m_hdrBuffer->GetRTV().GetAddressOf(), nullptr);
    }

    void GraphicsDevice::EndDrawLightPass()
    {
        m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);

        ID3D11ShaderResourceView* nullSRVs[m_gBuffer.count]{};
        m_deviceContext->PSSetShaderResources(m_gBuffer.startSlot, m_gBuffer.count, nullSRVs);
        m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::GBufferDepth), 1, nullSRVs);
        m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::ShadowMap), 1, nullSRVs);
    }

    void GraphicsDevice::BeginDrawForwardPass()
    {
        m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_deviceContext->RSSetViewports(1, &m_gameViewport);
        m_deviceContext->RSSetState(nullptr);

        m_deviceContext->OMSetRenderTargets(1, m_hdrBuffer->GetRTV().GetAddressOf(), m_gameDepthBuffer->GetRawDSV());
    }

    void GraphicsDevice::EndDrawForwardPass()
    {
        m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    }

    void GraphicsDevice::ProcessBloom()
    {
        m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        // 모든 단계에서 Linear Sampler 사용 (부드러운 다운샘플링)
        m_deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerLinear->GetSamplerState().GetAddressOf());

        ID3D11ShaderResourceView* nullSRV = nullptr;

        {
            D3D11_VIEWPORT vp{ 0, 0, (float)m_resolutionWidth / 2, (float)m_resolutionHeight / 2, 0, 1 };
            m_deviceContext->RSSetViewports(1, &vp);
            m_deviceContext->OMSetRenderTargets(1, m_bloomHalfBuffer->GetRTV().GetAddressOf(), nullptr);

            m_deviceContext->PSSetShader(m_brightPassPS->GetRawShader(), nullptr, 0);
            m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, m_hdrBuffer->GetSRV().GetAddressOf());

            DrawFullscreenQuad();

            m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
            m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, &nullSRV);
        }

        {
            m_deviceContext->OMSetRenderTargets(1, m_bloomWorkBuffer->GetRTV().GetAddressOf(), nullptr);
            m_deviceContext->PSSetShader(m_blurPS->GetRawShader(), nullptr, 0);
            m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, m_bloomHalfBuffer->GetSRV().GetAddressOf());

            CbBlur cb{};
            cb.blurDir = Vector2(1.0f / (m_resolutionWidth / 2.0f), 0.0f);
            m_deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Blur), 1, m_blurConstantBuffer->GetBuffer().GetAddressOf());
            m_deviceContext->UpdateSubresource(m_blurConstantBuffer->GetRawBuffer(), 0, nullptr, &cb, 0, 0);

            DrawFullscreenQuad();

            m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
            m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, &nullSRV);
        }

        {
            m_deviceContext->OMSetRenderTargets(1, m_bloomHalfBuffer->GetRTV().GetAddressOf(), nullptr);
            m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, m_bloomWorkBuffer->GetSRV().GetAddressOf());

            CbBlur cb{};
            cb.blurDir = Vector2(0.0f, 1.0f / (m_resolutionHeight / 2.0f));
            m_deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Blur), 1, m_blurConstantBuffer->GetBuffer().GetAddressOf());
            m_deviceContext->UpdateSubresource(m_blurConstantBuffer->GetRawBuffer(), 0, nullptr, &cb, 0, 0);

            DrawFullscreenQuad();

            m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
            m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, &nullSRV);
        }

        m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    }

    void GraphicsDevice::ProcessToneMapping()
    {
        m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerLinear->GetSamplerState().GetAddressOf());

        m_deviceContext->RSSetViewports(1, &m_gameViewport);
        m_deviceContext->RSSetState(nullptr);

        switch (m_format)
        {
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            m_deviceContext->PSSetShader(m_hdrPS->GetRawShader(), nullptr, 0);
            break;

        default:
            m_deviceContext->PSSetShader(m_ldrPS->GetRawShader(), nullptr, 0);
            break;
        }

        ID3D11ShaderResourceView* srvs[2]{ m_bloomHalfBuffer->GetRawSRV(), m_hdrBuffer->GetRawSRV() };

        m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 2, srvs);
        m_deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerLinear->GetSamplerState().GetAddressOf());

        m_deviceContext->OMSetRenderTargets(1, m_aaBuffer->GetRTV().GetAddressOf(), nullptr);

        DrawFullscreenQuad();

        m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);

        ID3D11ShaderResourceView* nullSRVs[2]{};
        m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 2, nullSRVs);
    }

    void GraphicsDevice::ProcessFXAA()
    {
        m_deviceContext->OMSetRenderTargets(1, m_finalBuffer->GetRTV().GetAddressOf(), nullptr);

        m_deviceContext->PSSetShader(m_fxaaPS->GetRawShader(), nullptr, 0);
        m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, m_aaBuffer->GetSRV().GetAddressOf());
        m_deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerLinear->GetSamplerState().GetAddressOf());

        DrawFullscreenQuad();

        ID3D11ShaderResourceView* nullSRV = nullptr;
        m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, &nullSRV);
    }

    void GraphicsDevice::ExecutePostProcessing()
    {
        ProcessBloom();
        ProcessToneMapping();
        ProcessFXAA();
    }

    void GraphicsDevice::BeginDrawScreenPass()
    {
        m_deviceContext->OMSetRenderTargets(1, m_finalBuffer->GetRTV().GetAddressOf(), nullptr);
    }

    void GraphicsDevice::EndDrawScreenPass()
    {
        m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    }

    void GraphicsDevice::BeginDrawOutlinePass()
    {
        m_deviceContext->OMSetRenderTargets(0, nullptr, m_outlineStencilBuffer->GetRawDSV());
        m_deviceContext->OMSetDepthStencilState(m_maskDSS->GetRawDepthStencilState(), 1);
    }

    void GraphicsDevice::EndDrawOutlinePass()
    {
        m_deviceContext->OMSetRenderTargets(1, m_finalBuffer->GetRTV().GetAddressOf(), nullptr);
        m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::StencilMap), 1, m_outlineStencilBuffer->GetSRV().GetAddressOf());

        m_deviceContext->PSSetShader(m_outlinePS->GetRawShader(), nullptr, 0);

        DrawFullscreenQuad();
        
        m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);

        ID3D11ShaderResourceView* nullSRV = nullptr;
        m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::StencilMap), 1, &nullSRV);
    }

    void GraphicsDevice::BeginDrawDebugPass()
    {
        // 디버그 렌더링용 패스 - finalBuffer에 렌더링, 깊이 테스트 없음
        m_deviceContext->RSSetViewports(1, &m_gameViewport);
        m_deviceContext->OMSetRenderTargets(1, m_finalBuffer->GetRTV().GetAddressOf(), nullptr);
    }

    void GraphicsDevice::EndDrawDebugPass()
    {
        m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    }

    void GraphicsDevice::BeginDrawGUIPass()
    {
        m_deviceContext->OMSetRenderTargets(1, m_finalBuffer->GetRTV().GetAddressOf(), nullptr);

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        io.DisplaySize = ImVec2((float)m_resolutionWidth, (float)m_resolutionHeight);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();
    }

    void GraphicsDevice::EndDrawGUIPass()
    {
        ImGui::Render();

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    }

    void GraphicsDevice::EndDraw()
    {

        m_deviceContext->RSSetViewports(1, &m_backBufferViewport);
        m_deviceContext->RSSetState(nullptr);

        m_deviceContext->PSSetShader(m_blitPS->GetRawShader(), nullptr, 0);
        m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, m_finalBuffer->GetSRV().GetAddressOf());
        m_deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerLinear->GetSamplerState().GetAddressOf());

        m_deviceContext->OMSetRenderTargets(1, m_backBufferRTV.GetAddressOf(), nullptr);

        DrawFullscreenQuad();

        m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);

        ID3D11ShaderResourceView* nullSRV = nullptr;
        m_deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, &nullSRV);

        m_swapChain->Present(m_syncInterval, m_presentFlags);

        // vram usage
        {
            DXGI_QUERY_VIDEO_MEMORY_INFO memInfo{};
            m_dxgiAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memInfo);
            Profiling::UpdateVRAMUsage(memInfo.CurrentUsage);
        }
    }

    const Microsoft::WRL::ComPtr<ID3D11Device>& GraphicsDevice::GetDevice() const
    {
        return m_device;
    }

    const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& GraphicsDevice::GetDeviceContext() const
    {
        return m_deviceContext;
    }

    const D3D11_VIEWPORT& GraphicsDevice::GetViewport() const
    {
        return m_gameViewport;
    }

    float GraphicsDevice::GetMaxHDRNits() const
    {
        return m_monitorMaxNits;
    }

    int GraphicsDevice::GetShadowMapSize() const
    {
        return m_shadowMapSize;
    }

    BufferTextures GraphicsDevice::GetBufferTextures() const
    {
        return {
            m_hdrBuffer->GetRawSRV(),
            m_gameDepthBuffer->GetRawSRV(),
            m_shadowDepthBuffer->GetRawSRV(),
            m_gBuffer.baseColor->GetRawSRV(),
            m_gBuffer.normal->GetRawSRV(),
            m_gBuffer.orm->GetRawSRV(),
            m_gBuffer.emissive->GetRawSRV(),
            m_bloomHalfBuffer->GetRawSRV(),
            m_bloomWorkBuffer->GetRawSRV(),
            m_aaBuffer->GetRawSRV()
        };
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
#ifdef _DEBUG
        DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
        shaderFlags |= D3DCOMPILE_DEBUG;
        shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;

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
#else
        std::filesystem::path csoPath = fileName;
        csoPath.replace_extension(".cso");

        HR_CHECK(D3DReadFileToBlob(csoPath.c_str(), &blobOut));

#endif // _DEBUG
    }

    void GraphicsDevice::CheckHDRSupportAndGetMaxNits()
    {
        Microsoft::WRL::ComPtr<IDXGIFactory4> pFactory;
        HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));

        // 2. 주 그래픽 어댑터 (0번) 열거
        Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter;
        UINT adapterIndex = 0;
        while (pFactory->EnumAdapters1(adapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC1 desc;
            pAdapter->GetDesc1(&desc);

            // WARP 어댑터(소프트웨어)를 건너뛰고 주 어댑터만 사용하도록 선택할 수 있습니다.
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                adapterIndex++;
                pAdapter.Reset();
                continue;
            }
            break;
        }

        // 3. 주 모니터 출력 (0번) 열거
        Microsoft::WRL::ComPtr<IDXGIOutput> pOutput;
        hr = pAdapter->EnumOutputs(0, &pOutput); // 0번 출력

        // 4. HDR 정보를 얻기 위해 IDXGIOutput6으로 쿼리
        Microsoft::WRL::ComPtr<IDXGIOutput6> pOutput6;
        hr = pOutput.As(&pOutput6);
        if (FAILED(hr))
        {
            // hdr 지원 x
            m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
            m_monitorMaxNits = 100.0f;
            m_isHDRSupported = false;

            return;
        }

        // 5. DXGI_OUTPUT_DESC1에서 HDR 정보 확인
        DXGI_OUTPUT_DESC1 desc1 = {};
        hr = pOutput6->GetDesc1(&desc1);

        // 6. HDR 활성화 조건 분석
        bool isHDRColorSpace = (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
        m_monitorMaxNits = (float)desc1.MaxLuminance;

        // OS가 HDR을 켰을 때 MaxLuminance는 100 Nits(SDR 기준)를 초과합니다.
        bool isHDRActive = m_monitorMaxNits > 100.0f;

        if (isHDRColorSpace && isHDRActive)
        {
            // 최종 판단: HDR 지원 및 OS 활성화
            m_format = DXGI_FORMAT_R10G10B10A2_UNORM; // HDR 포맷 설정
            m_isHDRSupported = true;
        }
        else
        {
            // HDR 지원 안함 또는 OS에서 비활성화
            m_monitorMaxNits = 100.0f; // SDR 기본값
            m_format = DXGI_FORMAT_R8G8B8A8_UNORM; // SDR 포맷 설정
            m_isHDRSupported = false;
        }
    }

    void GraphicsDevice::CreateCoreResources()
    {
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
                    &m_device,
                    &actualFeatureLevel,
                    &m_deviceContext));
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
                swapChainDesc.Width = m_isFullscreen ? m_screenWidth : m_resolutionWidth;
                swapChainDesc.Height = m_isFullscreen ? m_screenHeight : m_resolutionHeight;
                swapChainDesc.Format = m_format;
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
                    m_device.Get(),
                    m_hWnd,
                    &swapChainDesc,
                    nullptr,
                    nullptr,
                    &m_swapChain));
            }
        }

        // directx alt+enter 전체화면 전환 해제
        if (Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
            SUCCEEDED(m_swapChain->GetParent(__uuidof (IDXGIFactory1), &factory)))
        {
            factory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
        }
    }

    void GraphicsDevice::CreateShadowBuffer()
    {
        // shadow depth buffer
        {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = static_cast<UINT>(m_shadowMapSize);
            desc.Height = static_cast<UINT>(m_shadowMapSize);
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.Format = DXGI_FORMAT_R32_TYPELESS;
            desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

            m_shadowDepthBuffer = std::make_unique<Texture>();
            m_shadowDepthBuffer->Create(desc, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_D32_FLOAT);

            D3D11_RASTERIZER_DESC rsDesc{};
            rsDesc.FillMode = D3D11_FILL_SOLID;
            rsDesc.CullMode = D3D11_CULL_BACK;
            rsDesc.DepthBias = 5000;
            rsDesc.DepthBiasClamp = 0.0f;
            rsDesc.SlopeScaledDepthBias = 2.0f;
            rsDesc.DepthClipEnable = true;

            m_shadowMapRSS = std::make_unique<RasterizerState>();
            m_shadowMapRSS->Create(rsDesc);
        }

        // shadow viewport
        {
            m_shadowViewport.TopLeftX = 0;
            m_shadowViewport.TopLeftY = 0;
            m_shadowViewport.Width = static_cast<float>(m_shadowMapSize);
            m_shadowViewport.Height = static_cast<float>(m_shadowMapSize);
            m_shadowViewport.MinDepth = 0.0f;
            m_shadowViewport.MaxDepth = 1.0f;
        }
    }

    void GraphicsDevice::CreateSizeDependentResources()
    {
        // backbuffer
        {
            Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
            HR_CHECK(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
            HR_CHECK(m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_backBufferRTV));

            Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
            m_swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapChain3);
            if (m_format == DXGI_FORMAT_R10G10B10A2_UNORM)
            {
                swapChain3->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
            }
        }

        // final buffer
        {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = m_resolutionWidth;
            desc.Height = m_resolutionHeight;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

            m_finalBuffer = std::make_unique<Texture>();
            m_finalBuffer->Create(desc);
        }

        // aa buffer
        {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = m_resolutionWidth;
            desc.Height = m_resolutionHeight;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

            m_aaBuffer = std::make_unique<Texture>();
            m_aaBuffer->Create(desc);
        }

        // hdr buffer
        {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = m_resolutionWidth;
            desc.Height = m_resolutionHeight;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

            m_hdrBuffer = std::make_unique<Texture>();
            m_hdrBuffer->Create(desc);
        }

        // game depth buffer
        {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = m_resolutionWidth;
            desc.Height = m_resolutionHeight;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

            m_gameDepthBuffer = std::make_unique<Texture>();
            m_gameDepthBuffer->Create(desc, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_D24_UNORM_S8_UINT);
        }

        // G-Buffer
        {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = m_resolutionWidth;
            desc.Height = m_resolutionHeight;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

            m_gBuffer.baseColor = std::make_unique<Texture>();
            m_gBuffer.baseColor->Create(desc);

            desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;

            m_gBuffer.normal = std::make_unique<Texture>();
            m_gBuffer.normal->Create(desc);

            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

            m_gBuffer.orm = std::make_unique<Texture>();
            m_gBuffer.orm->Create(desc);

            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

            m_gBuffer.emissive= std::make_unique<Texture>();
            m_gBuffer.emissive->Create(desc);
        }

        // bloom textures
        {
            // 메모리 절약을 위해 R11G11B10_FLOAT 사용 권장 (없으면 R16G16B16A16_FLOAT)
            D3D11_TEXTURE2D_DESC desc{};
            desc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            desc.MipLevels = 1;
            desc.ArraySize = 1;

            // 1/2
            desc.Width = m_resolutionWidth / 2;
            desc.Height = m_resolutionHeight / 2;
            m_bloomHalfBuffer = std::make_unique<Texture>();
            m_bloomHalfBuffer->Create(desc);

            m_bloomWorkBuffer = std::make_unique<Texture>();
            m_bloomWorkBuffer->Create(desc);
        }

        // outline
        {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = m_resolutionWidth;
            desc.Height = m_resolutionHeight;
            desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
            desc.MipLevels = 1;
            desc.ArraySize = 1;

            m_outlineStencilBuffer = std::make_unique<Texture>();
            m_outlineStencilBuffer->Create(desc, DXGI_FORMAT_X24_TYPELESS_G8_UINT, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_D24_UNORM_S8_UINT);
        }

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

        if (m_isFullscreen)
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

        if (m_screenSizeCB)
        {
            CbScreenSize cb{
                Vector2(static_cast<float>(m_resolutionWidth), static_cast<float>(m_resolutionHeight)),
                0.0f, 0.0f };
            m_deviceContext->UpdateSubresource(m_screenSizeCB->GetRawBuffer(), 0, nullptr, &cb, 0, 0);
        }
    }

    void GraphicsDevice::CreateAdditionalResources()
    {
        // Create vertex, index buffer
        {
            auto geometryData = GeometryGenerator::MakeQuad(2.0f, 2.0f);

            m_quadVertexBuffer = std::make_shared<VertexBuffer>();
            m_quadVertexBuffer->Create(geometryData.vertices);

            m_quadIndexBuffer = std::make_shared<IndexBuffer>();
            m_quadIndexBuffer->Create(geometryData.indices);
        }

        // Create default shader, layout
        {
            m_fullscreenQuadVS = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/FullscreenQuad_VS.hlsl");
            m_quadInputLayout = m_fullscreenQuadVS->GetOrCreateInputLayout<PositionTexCoordVertex>();
        }

        // pixel shader
        {
            m_blitPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Blit_PS.hlsl");
            m_hdrPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/HDR_PS.hlsl");
            m_ldrPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/LDR_PS.hlsl");
            m_globalLightPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/DeferredGlobalLight_PS.hlsl");
            m_brightPassPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/BrightPass_PS.hlsl");
            m_blurPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Blur_PS.hlsl");
            m_fxaaPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/FXAA_PS.hlsl");
        }

        // Create Sampler State
        {
            m_samplerLinear = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);
            m_samplerPoint = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Point);
        }

        // Blur Constant Buffer
        {
            m_blurConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Blur", sizeof(CbBlur));
        }

        // mask depth stencil state
        {
            D3D11_DEPTH_STENCIL_DESC desc{};
            desc.DepthEnable = false;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            desc.DepthFunc = D3D11_COMPARISON_ALWAYS; // 혹은 ALWAYS (벽 뒤에 있는 것도 외곽선 따려면)

            // Stencil 설정 (핵심)
            desc.StencilEnable = true;
            desc.StencilReadMask = 0xFF;
            desc.StencilWriteMask = 0xFF;

            // 픽셀이 그려질 때 스텐실 버퍼에 무엇을 할 것인가?
            desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
            desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
            desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE; // <--- 통과하면 Ref 값으로 덮어씀 (Replace)
            desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;    // <--- 무조건 통과 (깊이 테스트만 통과한다면)

            // BackFace도 동일하게 설정
            desc.BackFace = desc.FrontFace;

            m_maskDSS = std::make_unique<DepthStencilState>();
            m_maskDSS->Create(desc);
        }

        // outline ps
        {
            m_outlinePS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Outline_PS.hlsl");
        }

        m_screenSizeCB = ResourceManager::Get().GetOrCreateConstantBuffer("ScreenSize", sizeof(CbScreenSize));
        m_deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::ScreenSize), 1, m_screenSizeCB->GetBuffer().GetAddressOf());
        CbScreenSize cb{
            Vector2(static_cast<float>(m_resolutionWidth), static_cast<float>(m_resolutionHeight)),
            0.0f, 0.0f };
        m_deviceContext->UpdateSubresource(m_screenSizeCB->GetRawBuffer(), 0, nullptr, &cb, 0, 0);
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
