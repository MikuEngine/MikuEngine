#include "../Include/Shared.hlsli"

PS_OUTPUT_GBUFFER main(PS_INPUT_TEXCOORD input)
{
    PS_OUTPUT_GBUFFER output = (PS_OUTPUT_GBUFFER) 0;
    
    float4 color = g_texBaseColor.Sample(g_samLinear, input.texCoord);
    
    clip(color.a - 0.5f);
    
    output.baseColor = float4(color.rgb * g_materialBaseColor.rgb, 1.0f);
    
    output.normal = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    return output;
}