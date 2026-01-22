#pragma once

#include <d3d11.h>
#include <directxtk/SimpleMath.h>

struct ParticleInstanceData
{
    DirectX::SimpleMath::Matrix world;
    DirectX::SimpleMath::Vector4 color;
    DirectX::SimpleMath::Vector4 uvOffsetScale;
};