#pragma once

#include "Core/Graphics/Data/ConstantBufferTypes.h"

namespace engine
{
    struct ProjectSettings
    {
        std::vector<std::string> sceneList;

        // 스카이박스/IBL 설정
        std::string skyboxTexturePath = "Resource/Texture/PlainsSunsetEnvHDR.dds";
        std::string iblIrradiancePath = "Resource/Texture/PlainsSunsetDiffuseHDR.dds";
        std::string iblSpecularPath = "Resource/Texture/PlainsSunsetSpecularHDR.dds";
        std::string iblBrdfLutPath = "Resource/Texture/PlainsSunsetBrdf.dds";
        
        Vector3 skyboxColor = {0.5f, 0.7f, 1.0f};
        Vector3 skyboxHorizonColor = {0.3f, 0.4f, 0.5f};
        Vector3 iblAmbientColor = {0.5f, 0.7f, 1.0f};
        
        bool useSkyboxTexture = true;
        bool useIBLTexture = true;
        bool useIBL = true;
        
        // 포스트프로세싱 설정
        float bloomStrength = 0.05f;
        float bloomThreshold = 1.0f;
        float bloomSoftKnee = 2.0f;
        bool enableBloom = true;
        
        float exposure = -2.5f;
        bool enableToneMapping = true;
        
        float fxaaQualitySubpix = 0.75f;
        float fxaaQualityEdgeThreshold = 0.166f;
        float fxaaQualityEdgeThresholdMin = 0.0833f;
        bool enableFXAA = true;

        void Save();
        void Load();
    };
}