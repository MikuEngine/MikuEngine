#pragma once

#include <filesystem>

#include "Common/Utility/Singleton.h"
#include "Core/Graphics/Data/ShaderSlotTypes.h"

namespace engine
{
    class Texture;
    class RasterizerState;
    class VertexBuffer;
    class IndexBuffer;
    class VertexShader;
    class PixelShader;
    class InputLayout;
    class SamplerState;
    class ConstantBuffer;
    class DepthStencilState;

    struct GBufferResources
    {
        std::unique_ptr<Texture> baseColor;
        std::unique_ptr<Texture> normal;
        std::unique_ptr<Texture> orm;
        std::unique_ptr<Texture> emissive;

        static constexpr UINT startSlot = static_cast<UINT>(TextureSlot::GBufferBaseColor);
        static constexpr UINT count = 4;

        void Reset();
        std::array<ID3D11RenderTargetView*, count> GetRawRTVs() const;
        std::array<ID3D11ShaderResourceView*, count> GetRawSRVs() const;
    };

    struct BufferTextures
    {
        ID3D11ShaderResourceView* hdr;
        ID3D11ShaderResourceView* gameDepth;
        ID3D11ShaderResourceView* shadowDepth;
        ID3D11ShaderResourceView* baseColor;
        ID3D11ShaderResourceView* normal;
        ID3D11ShaderResourceView* orm;
        ID3D11ShaderResourceView* emissive;
        ID3D11ShaderResourceView* bloomHalfBuffer;
        ID3D11ShaderResourceView* bloomWorkBuffer;
        ID3D11ShaderResourceView* aaBuffer;
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

        // bloom
        std::unique_ptr<Texture> m_bloomHalfBuffer;
        std::unique_ptr<Texture> m_bloomWorkBuffer;

        // fxaa
        std::unique_ptr<Texture> m_aaBuffer;

        // outline
        std::unique_ptr<Texture> m_outlineStencilBuffer;
        std::unique_ptr<DepthStencilState> m_maskDSS;

        // picking
        std::unique_ptr<Texture> m_pickingIdBuffer;
        std::unique_ptr<Texture> m_stagingIdBuffer;

        std::unique_ptr<RasterizerState> m_shadowMapRSS;

        GBufferResources m_gBuffer;

        Microsoft::WRL::ComPtr<IDXGIAdapter3> m_dxgiAdapter;

        // shaders
        std::shared_ptr<VertexShader> m_fullscreenQuadVS;
        std::shared_ptr<PixelShader> m_blitPS;
        std::shared_ptr<PixelShader> m_globalLightPS;
        std::shared_ptr<PixelShader> m_hdrPS;
        std::shared_ptr<PixelShader> m_ldrPS;
        std::shared_ptr<PixelShader> m_brightPassPS;
        std::shared_ptr<PixelShader> m_blurPS;
        std::shared_ptr<PixelShader> m_fxaaPS;
        std::shared_ptr<PixelShader> m_outlinePS;

        std::shared_ptr<InputLayout> m_quadInputLayout;
        std::shared_ptr<SamplerState> m_samplerLinear;
        std::shared_ptr<SamplerState> m_samplerPoint;

        std::shared_ptr<ConstantBuffer> m_blurConstantBuffer;

        std::shared_ptr<ConstantBuffer> m_screenSizeCB;

        // quad
        std::unique_ptr<VertexBuffer> m_quadVertexBuffer;
        std::unique_ptr<IndexBuffer> m_quadIndexBuffer;

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
        void InitializeResources();
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

        void BeginDrawEditorPass();
        void EndDrawEditorPass();

        void BeginDrawForwardPass();
        void EndDrawForwardPass();

        void ExecutePostProcessing();

        void BeginDrawScreenPass();
        void EndDrawScreenPass();

        void BeginDrawOutlinePass();
        void EndDrawOutlinePass();

        void BeginDrawDebugPass();
        void EndDrawDebugPass();

        void BeginDrawGUIPass();
        void EndDrawGUIPass();

        void BeginDrawPickingPass();
        unsigned int EndDrawPickingPass(int mouseX, int mouseY);

        void EndDraw();

        const Microsoft::WRL::ComPtr<ID3D11Device>& GetDevice() const;
        const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& GetDeviceContext() const;
        const D3D11_VIEWPORT& GetViewport() const;
        float GetMaxHDRNits() const;
        int GetShadowMapSize() const;
        BufferTextures GetBufferTextures() const;

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
        void CreateAdditionalResources();
        Microsoft::WRL::ComPtr<IDXGIAdapter1> GetHighPerformanceAdapter(Microsoft::WRL::ComPtr<IDXGIFactory5> factory);

        void ProcessBloom();
        void ProcessToneMapping();
        void ProcessFXAA();

    private:
        friend class Singleton<GraphicsDevice>;
    };
}
