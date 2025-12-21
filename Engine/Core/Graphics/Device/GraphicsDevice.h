#pragma once

#include <filesystem>

#include "Common/Utility/GeometryGenerator.h"
#include "Common/Utility/Singleton.h"

namespace engine
{
    struct DeviceResources;

    class GraphicsDevice :
        public Singleton<GraphicsDevice>
    {
    private:
        std::unique_ptr<DeviceResources> m_resource;

        HWND m_hWnd = nullptr;
        UINT m_resolutionWidth = 0;
        UINT m_resolutionHeight = 0;
        UINT m_screenWidth = 0;
        UINT m_screenHeight = 0;
        D3D11_VIEWPORT m_gameViewport{};
        D3D11_VIEWPORT m_backBufferViewport{};

        UINT m_syncInterval = 0;
        UINT m_presentFlags = 0;
        bool m_useVsync = false;
        bool m_tearingSupport = false;
        bool m_isFullScreen = false;

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
            bool isFullScreen,
            bool useVsync);
        void Shutdown();

    public:
        bool Resize(UINT resolutionWidth,
            UINT resolutionHeight,
            UINT screenWidth,
            UINT screenHeight,
            bool isFullScreen);
        void BeginDraw(const Color& clearColor = {});
        void BackBufferDraw();
        void EndDraw();

        const Microsoft::WRL::ComPtr<ID3D11Device>& GetDevice() const;
        const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& GetDeviceContext() const;
        const Microsoft::WRL::ComPtr<IDXGISwapChain1>& GetSwapChain() const;
        const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& GetRenderTargetView() const;
        const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& GetDepthStencilView() const;
        const D3D11_VIEWPORT& GetViewport() const;

        void SetVsync(bool useVsync);

    public:
        static void CompileShaderFromFile(
            const std::filesystem::path& fileName,
            const std::string& entryPoint,
            const std::string& shaderModel,
            Microsoft::WRL::ComPtr<ID3DBlob>& blobOut);

    private:
        void CreateSizeDependentResources();
        Microsoft::WRL::ComPtr<IDXGIAdapter1> GetHighPerformanceAdapter(Microsoft::WRL::ComPtr<IDXGIFactory5> factory);

    private:
        friend class Singleton<GraphicsDevice>;
    };
}
