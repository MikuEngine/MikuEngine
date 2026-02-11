#include "EnginePCH.h"
#include "Texture.h"

#include <directxtk/WICTextureLoader.h>
#include <directxtk/DDSTextureLoader.h>
#include <DirectXTex.h>
#include <filesystem>

#include "Core/Graphics/Device/GraphicsDevice.h"
#include "Common/Utility/TextureHelper.h"
#include "Core/System/VirtualFileSystem.h"

namespace engine
{
    namespace
    {
        const D3D11_TEXTURE2D_DESC g_defaultTexDesc{
            1, 1, 1, 1,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            { 1, 0 },
            D3D11_USAGE_IMMUTABLE,
            D3D11_BIND_SHADER_RESOURCE,
            0, 0
        };
    }

    void Texture::Create(const std::string& filePath)
    {
        namespace fs = std::filesystem;

        auto path = fs::path(filePath);
        auto extension = path.extension();

        const auto& device = GraphicsDevice::Get().GetDevice();
        auto& vfs = VirtualFileSystem::Get();

        // VFS를 통해 파일 로드
        std::vector<uint8_t> fileData;
        if (!vfs.LoadFile(filePath, fileData))
        {
            LOG_ERROR("텍스처 파일 로드 실패: {} - Texture", filePath);
            return;
        }

        if (extension == ".tga" || extension == ".TGA")
        {
            DirectX::ScratchImage image;
            HR_CHECK(LoadFromTGAMemory(fileData.data(), fileData.size(), nullptr, image));

            HR_CHECK(CreateShaderResourceView(
                device.Get(),
                image.GetImages(),
                image.GetImageCount(),
                image.GetMetadata(),
                &m_srv));
        }
        //// exr 파일 쓰면 활성화 및 라이브러리 설치 필요함
        //else if (extension == ".exr" || extension == ".EXR")
        //{
        //    // 1. 파일 경로를 std::string으로 변환 (OpenEXR이 std::string/char*를 선호)
        //    std::string path_s = fs::path(filePath).string();

        //    int width = 0;
        //    int height = 0;

        //    // OpenEXR 배열을 위한 메모리 할당
        //    Imf::Array2D<Imf::Rgba> pixels;

        //    // 2. EXR 파일 읽기 (헤더 읽기 및 픽셀 데이터 읽기)
        //    Imf::RgbaInputFile file(path_s.c_str());
        //    const Imath::Box2i& dw = file.header().dataWindow();

        //    width = dw.max.x - dw.min.x + 1;
        //    height = dw.max.y - dw.min.y + 1;

        //    // 메모리 할당
        //    pixels.resizeErase(height, width);

        //    // 픽셀 읽기
        //    // frameBuffer 설정: 1은 x-stride, width는 y-stride
        //    file.setFrameBuffer(&pixels[0][0], 1, width);
        //    file.readPixels(dw.min.y, dw.max.y);

        //    // 3. DirectXTex ScratchImage 초기화
        //    DirectX::ScratchImage image;
        //    // EXR Rgba는 16비트 Half Float이므로 R16G16B16A16_FLOAT 포맷 사용
        //    const DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT;

        //    // ScratchImage 메모리 할당
        //    image.Initialize2D(format, width, height, 1, 1);

        //    // ScratchImage의 첫 번째 이미지(서브리소스)에 대한 포인터
        //    const DirectX::Image* img = image.GetImage(0, 0, 0);

        //    // 4. OpenEXR 데이터를 ScratchImage 메모리로 복사
        //    // OpenEXR Rgba 구조체는 R, G, B, A 순서로 4개의 Half(16비트 float)를 가집니다.
        //    // 이는 R16G16B16A16_FLOAT와 메모리 레이아웃이 일치하므로 직접 복사가 가능합니다.
        //    // Imf::Rgba는 총 8바이트 (4 * 2바이트 Half)
        //    size_t rowPitch = width * sizeof(Imf::Rgba);
        //    size_t slicePitch = height * rowPitch;

        //    // 픽셀 데이터 복사 (Rgba 배열을 ScratchImage 데이터로 복사)
        //    memcpy(img->pixels, pixels[0], slicePitch);

        //    // 5. D3D11 리소스 생성
        //    // ScratchImage 객체가 이제 유효한 텍스처 데이터를 가지고 있습니다.
        //    DirectX::CreateShaderResourceView(
        //        device.Get(),
        //        image.GetImages(),
        //        image.GetImageCount(),
        //        image.GetMetadata(),
        //        &m_shaderResourceView
        //    );
        //}
        else if (extension == ".dds" || extension == ".DDS")
        {
            HR_CHECK(DirectX::CreateDDSTextureFromMemory(
                device.Get(),
                fileData.data(),
                fileData.size(),
                nullptr,
                &m_srv));
        }
        else
        {
            HR_CHECK(DirectX::CreateWICTextureFromMemory(
                device.Get(),
                fileData.data(),
                fileData.size(),
                nullptr,
                &m_srv));
        }

        m_desc = GetTextureDescFromSRV(m_srv.Get());
    }

    void Texture::Create(UINT width, UINT height, DXGI_FORMAT format, UINT bindFlags)
    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.Format = format;
        desc.BindFlags = bindFlags;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;

        Create(desc);
    }

    void Texture::Create(
        const D3D11_TEXTURE2D_DESC& desc,
        DXGI_FORMAT srvFormat,
        DXGI_FORMAT rtvFormat,
        DXGI_FORMAT dsvFormat)
    {
        const auto& device = GraphicsDevice::Get().GetDevice();

        m_desc = desc;
        HR_CHECK(device->CreateTexture2D(&desc, nullptr, &m_texture));

        bool isCube = (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0;
        bool isCubeArray = isCube && (desc.ArraySize > 6); // 6장 초과면 큐브 배열
        bool isArray = desc.ArraySize > 1;

        if (m_desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = (srvFormat != DXGI_FORMAT_UNKNOWN) ? srvFormat : desc.Format;

            if (isCubeArray)
            {
                // [Cube Array]
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
                srvDesc.TextureCubeArray.MostDetailedMip = 0;
                srvDesc.TextureCubeArray.MipLevels = m_desc.MipLevels;
                srvDesc.TextureCubeArray.First2DArrayFace = 0;
                srvDesc.TextureCubeArray.NumCubes = desc.ArraySize / 6; // 큐브 개수
            }
            else if (isCube)
            {
                // [Single Cube]
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                srvDesc.TextureCube.MostDetailedMip = 0;
                srvDesc.TextureCube.MipLevels = m_desc.MipLevels;
            }
            else if (isArray)
            {
                // [Texture2D Array]
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.MostDetailedMip = 0;
                srvDesc.Texture2DArray.MipLevels = m_desc.MipLevels;
                srvDesc.Texture2DArray.FirstArraySlice = 0;
                srvDesc.Texture2DArray.ArraySize = desc.ArraySize;
            }
            else
            {
                // [Texture2D]
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Texture2D.MipLevels = m_desc.MipLevels;
            }

            HR_CHECK(device->CreateShaderResourceView(m_texture.Get(), &srvDesc, &m_srv));
        }

        if (m_desc.BindFlags & D3D11_BIND_RENDER_TARGET)
        {
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
            rtvDesc.Format = (rtvFormat != DXGI_FORMAT_UNKNOWN) ? rtvFormat : desc.Format;

            if (isArray || isCube)
            {
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                rtvDesc.Texture2DArray.MipSlice = 0;
                rtvDesc.Texture2DArray.FirstArraySlice = 0;
                rtvDesc.Texture2DArray.ArraySize = desc.ArraySize; // 전체 장수 (CubeArray면 6 * N)
            }
            else
            {
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = 0;
            }

            HR_CHECK(device->CreateRenderTargetView(m_texture.Get(), &rtvDesc, &m_rtv));
        }

        if (m_desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
            dsvDesc.Format = (dsvFormat != DXGI_FORMAT_UNKNOWN) ? dsvFormat : desc.Format;

            if (isArray && !isCube)
            {
                // Per-slice DSV for 2D array (e.g. spot shadow maps) so we can render to one slice at a time
                m_dsvArray.resize(desc.ArraySize);
                dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                dsvDesc.Texture2DArray.MipSlice = 0;
                dsvDesc.Texture2DArray.ArraySize = 1;
                for (UINT i = 0; i < desc.ArraySize; ++i)
                {
                    dsvDesc.Texture2DArray.FirstArraySlice = i;
                    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> dsv;
                    HR_CHECK(device->CreateDepthStencilView(m_texture.Get(), &dsvDesc, &dsv));
                    m_dsvArray[i] = std::move(dsv);
                }
            }
            else if (isCube)
            {
                dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                dsvDesc.Texture2DArray.MipSlice = 0;
                dsvDesc.Texture2DArray.FirstArraySlice = 0;
                dsvDesc.Texture2DArray.ArraySize = desc.ArraySize;
                HR_CHECK(device->CreateDepthStencilView(m_texture.Get(), &dsvDesc, &m_dsv));
            }
            else
            {
                dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                dsvDesc.Texture2D.MipSlice = 0;
                HR_CHECK(device->CreateDepthStencilView(m_texture.Get(), &dsvDesc, &m_dsv));
            }
        }
    }

    void Texture::Create(const std::array<unsigned char, 4>& color)
    {
        const auto& device = GraphicsDevice::Get().GetDevice();

        D3D11_SUBRESOURCE_DATA subData{};
        subData.pSysMem = color.data();
        subData.SysMemPitch = static_cast<UINT>(color.size());

        HR_CHECK(device->CreateTexture2D(&g_defaultTexDesc, &subData, &m_texture));

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = g_defaultTexDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        HR_CHECK(device->CreateShaderResourceView(m_texture.Get(), &srvDesc, &m_srv));
    }

    bool Texture::ReloadFromFile(const std::string& filePath)
    {
        Texture reloaded;
        reloaded.Create(filePath);

        if (reloaded.GetRawSRV() == nullptr)
        {
            LOG_ERROR("텍스처 리로드 실패. 기존 텍스처 유지: {}", filePath);
            return false;
        }

        *this = std::move(reloaded);
        return true;
    }

    const Microsoft::WRL::ComPtr<ID3D11Texture2D>& Texture::GetTexture() const
    {
        return m_texture;
    }

    const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& Texture::GetSRV() const
    {
        return m_srv;
    }

    const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& Texture::GetRTV() const
    {
        return m_rtv;
    }

    const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& Texture::GetDSV() const
    {
        return m_dsv;
    }

    ID3D11Texture2D* Texture::GetRawTexture() const
    {
        return m_texture.Get();
    }

    ID3D11ShaderResourceView* Texture::GetRawSRV() const
    {
        return m_srv.Get();
    }

    ID3D11RenderTargetView* Texture::GetRawRTV() const
    {
        return m_rtv.Get();
    }

    ID3D11DepthStencilView* Texture::GetRawDSV() const
    {
        if (!m_dsvArray.empty())
            return m_dsvArray[0].Get();
        return m_dsv.Get();
    }

    ID3D11DepthStencilView* Texture::GetRawDSV(UINT arraySlice) const
    {
        if (!m_dsvArray.empty())
            return arraySlice < m_dsvArray.size() ? m_dsvArray[arraySlice].Get() : nullptr;
        return m_dsv.Get();
    }

    float Texture::GetWidth() const
    {
        return static_cast<float>(m_desc.Width);
    }

    float Texture::GetHeight() const
    {
        return static_cast<float>(m_desc.Height);
    }
}