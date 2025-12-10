#pragma once

namespace engine
{
	class GraphicsDevice
	{
	private:
		Microsoft::WRL::ComPtr<ID3D11Device> m_d3d11Device;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3d11DeviceContext;
		Microsoft::WRL::ComPtr<IDXGISwapChain1> m_dxgiSwapChain;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_d3d11RenderTargetView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_d3d11DepthStencilView;

		Microsoft::WRL::ComPtr<IDXGIAdapter3> m_dxgiAdapter;

		HWND m_hWnd = nullptr;
		UINT m_width = 0;
		UINT m_height = 0;
		D3D11_VIEWPORT m_viewport{};

		// vsync
		UINT m_syncInterval = 0;
		UINT m_presentFlags = 0;
		bool m_useVsync = false;
		bool m_tearingSupport = false;

	public:
		void Initialize(HWND hWnd, UINT width, UINT height, bool useVsync);
		bool Resize(UINT width, UINT height);
		void BeginDraw(const Color& clearColor);
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
			const std::string& fileName,
			const std::string& entryPoint,
			const std::string& shaderModel,
			Microsoft::WRL::ComPtr<ID3DBlob>& blobOut);

	private:
		void CreateSizeDependentResources();
	};
}