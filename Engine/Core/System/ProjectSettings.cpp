#include "EnginePCH.h"
#include "ProjectSettings.h"

#include <fstream>
#include <filesystem>
#include <iomanip>

#include "Common/Utility/JsonHelper.h"
#include "Core/System/VirtualFileSystem.h"

namespace engine
{
    namespace
    {
        const char* g_settingPath = "Resource/Setting/Project.setting";
    }

    void ProjectSettings::Save()
    {
        json root;

        root["SceneList"] = sceneList;
        
        // 스카이박스/IBL 설정
        root["SkyboxTexturePath"] = skyboxTexturePath;
        root["IBLIrradiancePath"] = iblIrradiancePath;
        root["IBLSpecularPath"] = iblSpecularPath;
        root["IBLBrdfLutPath"] = iblBrdfLutPath;
        root["SkyboxColor"] = skyboxColor;
        root["SkyboxHorizonColor"] = skyboxHorizonColor;
        root["IBLAmbientColor"] = iblAmbientColor;
        root["UseSkyboxTexture"] = useSkyboxTexture;
        root["UseIBLTexture"] = useIBLTexture;
        root["UseIBL"] = useIBL;
        
        // 포스트프로세싱 설정
        root["BloomStrength"] = bloomStrength;
        root["BloomThreshold"] = bloomThreshold;
        root["BloomSoftKnee"] = bloomSoftKnee;
        root["EnableBloom"] = enableBloom;
        root["Exposure"] = exposure;
        root["EnableToneMapping"] = enableToneMapping;
        root["FXAAQualitySubpix"] = fxaaQualitySubpix;
        root["FXAAQualityEdgeThreshold"] = fxaaQualityEdgeThreshold;
        root["FXAAQualityEdgeThresholdMin"] = fxaaQualityEdgeThresholdMin;
        root["EnableFXAA"] = enableFXAA;

        std::filesystem::path path{ g_settingPath };

        if (path.has_parent_path())
        {
            std::filesystem::create_directories(path.parent_path());
        }

        std::ofstream o{ path };
        if (o.is_open())
        {
            o << std::setw(4) << root << std::endl;
        }
    }

    void ProjectSettings::Load()
    {
        // VFS를 통해 파일 로드 시도
        auto& vfs = VirtualFileSystem::Get();
        std::vector<uint8_t> fileData;
        
        if (vfs.LoadFile(g_settingPath, fileData))
        {
            try
            {
                json root = json::parse(fileData.begin(), fileData.end());
                JsonGet(root, "SceneList", sceneList);
                
                // 스카이박스/IBL 설정
                JsonGet(root, "SkyboxTexturePath", skyboxTexturePath);
                JsonGet(root, "IBLIrradiancePath", iblIrradiancePath);
                JsonGet(root, "IBLSpecularPath", iblSpecularPath);
                JsonGet(root, "IBLBrdfLutPath", iblBrdfLutPath);
                JsonGet(root, "SkyboxColor", skyboxColor);
                JsonGet(root, "SkyboxHorizonColor", skyboxHorizonColor);
                JsonGet(root, "IBLAmbientColor", iblAmbientColor);
                JsonGet(root, "UseSkyboxTexture", useSkyboxTexture);
                JsonGet(root, "UseIBLTexture", useIBLTexture);
                JsonGet(root, "UseIBL", useIBL);
                
                // 포스트프로세싱 설정
                JsonGet(root, "BloomStrength", bloomStrength);
                JsonGet(root, "BloomThreshold", bloomThreshold);
                JsonGet(root, "BloomSoftKnee", bloomSoftKnee);
                JsonGet(root, "EnableBloom", enableBloom);
                JsonGet(root, "Exposure", exposure);
                JsonGet(root, "EnableToneMapping", enableToneMapping);
                JsonGet(root, "FXAAQualitySubpix", fxaaQualitySubpix);
                JsonGet(root, "FXAAQualityEdgeThreshold", fxaaQualityEdgeThreshold);
                JsonGet(root, "FXAAQualityEdgeThresholdMin", fxaaQualityEdgeThresholdMin);
                JsonGet(root, "EnableFXAA", enableFXAA);
            }
            catch (const json::parse_error& e)
            {
                LOG_ERROR("ProjectSettings 파일 파싱 실패: {} - ProjectSettings", e.what());
            }
        }
        else
        {
            // VFS에서 실패하면 파일 시스템에서 시도 (개발 모드)
            std::filesystem::path path{ g_settingPath };
            if (std::filesystem::exists(path))
            {
                std::ifstream i{ path };
                if (i.is_open())
                {
                    try
                    {
                        json root;
                        i >> root;
                        JsonGet(root, "SceneList", sceneList);
                        
                        // 스카이박스/IBL 설정
                        JsonGet(root, "SkyboxTexturePath", skyboxTexturePath);
                        JsonGet(root, "IBLIrradiancePath", iblIrradiancePath);
                        JsonGet(root, "IBLSpecularPath", iblSpecularPath);
                        JsonGet(root, "IBLBrdfLutPath", iblBrdfLutPath);
                        JsonGet(root, "SkyboxColor", skyboxColor);
                        JsonGet(root, "SkyboxHorizonColor", skyboxHorizonColor);
                        JsonGet(root, "IBLAmbientColor", iblAmbientColor);
                        JsonGet(root, "UseSkyboxTexture", useSkyboxTexture);
                        JsonGet(root, "UseIBLTexture", useIBLTexture);
                        JsonGet(root, "UseIBL", useIBL);
                        
                        // 포스트프로세싱 설정
                        JsonGet(root, "BloomStrength", bloomStrength);
                        JsonGet(root, "BloomThreshold", bloomThreshold);
                        JsonGet(root, "BloomSoftKnee", bloomSoftKnee);
                        JsonGet(root, "EnableBloom", enableBloom);
                        JsonGet(root, "Exposure", exposure);
                        JsonGet(root, "EnableToneMapping", enableToneMapping);
                        JsonGet(root, "FXAAQualitySubpix", fxaaQualitySubpix);
                        JsonGet(root, "FXAAQualityEdgeThreshold", fxaaQualityEdgeThreshold);
                        JsonGet(root, "FXAAQualityEdgeThresholdMin", fxaaQualityEdgeThresholdMin);
                        JsonGet(root, "EnableFXAA", enableFXAA);
                    }
                    catch (const json::parse_error& e)
                    {
                        LOG_ERROR("ProjectSettings 파일 파싱 실패: {} - ProjectSettings", e.what());
                    }
                }
            }
        }
    }
}