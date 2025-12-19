#pragma once

enum class DefaultTextureType
{
    White,
    Black,
    Normal,
    Count
};

enum class DefaultSamplerType
{
    Point,
    Linear,
    Anisotropic,
    Comparison,
    Count
};

enum class DefaultRasterizerType
{
    SolidBack,
    SolidFront,
    SolidNone,
    Wireframe,
    Count
};

enum class DefaultDepthStencilType
{
    Less,
    LessEqual,
    DepthRead,
    None,
    Count
};

enum class DefaultBlendType
{
    Disabled,
    AlphaBlend,
    Additive,
    Count
};