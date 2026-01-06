#include "../Include/Shared.hlsli"

PS_OUTPUT_GBUFFER main(PS_INPUT_TEXCOORD input)
{
    PS_OUTPUT_GBUFFER output = (PS_OUTPUT_GBUFFER) 0;
    
    output.baseColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 color = g_texBaseColor.Sample(g_samLinear, input.texCoord);
    output.emissive = float4(color.rgb, 1.0f);
    
    output.orm = float4(0.0f, 1.0f, 0.0f, 0.0f);
    
    if (g_overrideMaterial)
    {
        output.emissive = g_materialBaseColor;
    }
    
    output.normal = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    return output;
}