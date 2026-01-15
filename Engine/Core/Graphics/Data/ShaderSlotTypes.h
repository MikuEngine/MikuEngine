#pragma once

enum class TextureSlot
{
    BaseColor               = 0,
    Normal                  = 1,
    Metalness               = 2,
    Roughness               = 3,
    AmbientOcclusion        = 4,
    Emissive                = 5,

    GBufferBaseColor        = 10,
    GBufferNormal           = 11,
    GBufferORM              = 12,
    GBufferEmissive         = 13,
    GBufferDepth            = 14,

    ShadowMap               = 20,
    IBLEnvironment          = 21,
    IBLIrradiance           = 22,
    IBLSpecular             = 23,
    IBLSpecularBRDFLUT      = 24,

    Blit                    = 30,
    HDR                     = 31,
    StencilMap              = 32,
};

enum class SamplerSlot
{
    Linear = 0,
    Point = 1,
    Comparison = 2,
    Clamp = 3
};

enum class ConstantBufferSlot
{
    Frame = 0,
    Material = 1,
    Object = 2,
    Bone = 3,
    Blur = 4,
    Sprite = 5,
    LocalLight = 6,
    ScreenSize = 7,
    Grid = 8,
};