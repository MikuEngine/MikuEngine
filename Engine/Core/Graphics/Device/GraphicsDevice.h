#pragma once

#include <filesystem>

#include "Common/Utility/Singleton.h"

namespace engine
{
    class Texture;
    class RasterizerState;

    struct GBufferResources
    {
        std::unique_ptr<Texture> baseColor;
        std::unique_ptr<Texture> position;
        std::unique_ptr<Texture> normal;
        std::unique_ptr<Texture> orm;
        std::unique_ptr<Texture> emissive;

        void Reset();
        std::array<ID3D11RenderTargetView*, 5> GetRawRTVs() const;
        std::array<ID3D11ShaderResourceView*, 5> GetRawSRVs() const;
    };

    class GraphicsDevice :
        public Singleton<GraphicsDevice>
    {
    private:
        Microsoft::WRL::ComPtr<ID3D11Device> m_device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_deviceContext;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;

        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_backBufferRTV;

        std::unique_ptr<Texture> m_finalBuffer;
        std::unique_ptr<Texture> m_hdrBuffer;
        std::unique_ptr<Texture> m_gameDepthBuffer;
        std::unique_ptr<Texture> m_shadowDepthBuffer;

        std::unique_ptr<RasterizerState> m_shadowMapRSS;

        GBufferResources m_gBuffer;

        Microsoft::WRL::ComPtr<IDXGIAdapter3> m_dxgiAdapter;

        // Resources for Blit
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_fullscreenQuadVS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_blitPS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_globalLightPS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_hdrPS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ldrPS;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_quadInputLayout;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerLinear;

        // quad
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_quadVertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_quadIndexBuffer;
        UINT m_quadVertexCount = 0;
        UINT m_quadIndexCount = 0;
        UINT m_quadVertexStride = 0;

        HWND m_hWnd = nullptr;
        UINT m_resolutionWidth = 0;
        UINT m_resolutionHeight = 0;
        UINT m_screenWidth = 0;
        UINT m_screenHeight = 0;
        D3D11_VIEWPORT m_gameViewport{};
        D3D11_VIEWPORT m_backBufferViewport{};
        D3D11_VIEWPORT m_shadowViewport{};

        UINT m_syncInterval = 0;
        UINT m_presentFlags = 0;
        int m_shadowMapSize = 8192;
        bool m_useVsync = false;
        bool m_tearingSupport = false;
        bool m_isFullscreen = false;
        DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
        float m_monitorMaxNits = 0.0f;
        bool m_forceLDR = false;
        bool m_isHDRSupported = false;

    private:
        GraphicsDevice();
        ~GraphicsDevice();

    public:
        void Initialize(
            HWND hWnd,
            UINT resolutionWidth,
            UINT resolutionHeight,
            UINT screenWidth,
            UINT screenHeight,
            bool isFullscreen,
            bool useVsync);
        void Shutdown();

    public:
        bool Resize(UINT resolutionWidth,
            UINT resolutionHeight,
            UINT screenWidth,
            UINT screenHeight,
            bool isFullscreen);

        void DrawFullscreenQuad();

        void ClearAllViews();

        void BeginDrawShadowPass();
        void EndDrawShadowPass();

        void BeginDrawGeometryPass();
        void EndDrawGeometryPass();

        void BeginDrawLightPass();
        void EndDrawLightPass();

        void BeginDrawForwardPass();
        void EndDrawForwardPass();

        void BeginDrawPostProccessingPass();
        void EndDrawPostProccessingPass();

        void BeginDrawGUIPass();
        void EndDrawGUIPass();

        void EndDraw();

        const Microsoft::WRL::ComPtr<ID3D11Device>& GetDevice() const;
        const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& GetDeviceContext() const;
        const D3D11_VIEWPORT& GetViewport() const;
        float GetMaxHDRNits() const;
        int GetShadowMapSize() const;

        void SetVsync(bool useVsync);

    public:
        static void CompileShaderFromFile(
            const std::filesystem::path& fileName,
            const std::string& entryPoint,
            const std::string& shaderModel,
            Microsoft::WRL::ComPtr<ID3DBlob>& blobOut);

    private:
        void CheckHDRSupportAndGetMaxNits();
        void CreateCoreResources();
        void CreateShadowBuffer();
        void CreateSizeDependentResources();
        void CreateFullscreenQuadResources();
        Microsoft::WRL::ComPtr<IDXGIAdapter1> GetHighPerformanceAdapter(Microsoft::WRL::ComPtr<IDXGIFactory5> factory);

    private:
        friend class Singleton<GraphicsDevice>;
    };
}
