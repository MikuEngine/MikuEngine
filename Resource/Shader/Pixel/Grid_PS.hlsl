#include "../Include/Shared.hlsli"

float4 main(PS_INPUT_WORLD_POSITION input) : SV_Target
{
    float2 coord = input.worldPosition.xz / g_gridSpacing;
    float2 derivative = fwidth(coord);
    float2 grid = abs(frac(coord - 0.5f) - 0.5f) / derivative;
    float lineDist = min(grid.x, grid.y);
    
    float minorAlpha = saturate(g_gridWidth - lineDist) * 0.3f;
    
    float majorSpacing = 10.0f;
    
    float2 coordMajor = input.worldPosition.xz / (g_gridSpacing * majorSpacing);
    float2 derivativeMajor = fwidth(coordMajor);
    float2 gridMajor = abs(frac(coordMajor - 0.5f) - 0.5f) / derivativeMajor;
    float lineDistMajor = min(gridMajor.x, gridMajor.y);
    
    float majorAlpha = saturate(g_gridWidth - lineDistMajor);
    
    float pixelVal = max(minorAlpha, majorAlpha);

    float4 color = g_gridColor;
    color.a = pixelVal;
    
    float2 axisDist = abs(input.worldPosition.xz) / (derivative * g_gridSpacing);
    
    if (axisDist.x < g_gridWidth)
    {
        color.rgb = float3(0.1f, 0.1f, 2.0f);
        color.a = saturate(g_gridWidth - axisDist.x);
    }
    
    if (axisDist.y < g_gridWidth)
    {
        color.rgb = float3(2.0f, 0.1f, 0.1f);
        color.a = saturate(g_gridWidth - axisDist.y);
    }

    float dist = distance(input.worldPosition.xyz, g_cameraWorldPosition.xyz);
    
    if (dist < 0.1f)
    {
        discard;
    }
    
    float fadeStart = 10.0f;
    float fadeEnd = 300.0f;
    
    float linearFade = 1.0f - saturate((dist - fadeStart) / (fadeEnd - fadeStart));
    color.a *= linearFade;

    return color;
}