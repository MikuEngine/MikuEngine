#pragma once

namespace engine
{
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
        Clamp,
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
        AlphaBlendPremultiplied,  // src = (rgb*a, a) 출력 시 One, InvSrcAlpha
        Additive,
        Count
    };
}