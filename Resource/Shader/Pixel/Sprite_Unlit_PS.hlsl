#include "../Include/Shared.hlsli"

PS_OUTPUT_GBUFFER main(PS_INPUT_TEXCOORD input)
{
    PS_OUTPUT_GBUFFER output = (PS_OUTPUT_GBUFFER) 0;
    
    output.baseColor = g_texBaseColor.Sample(g_samLinear, input.texCoord);
    output.baseColor.rgb *= g_materialBaseColor.rgb;
    
    output.normal = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    return output;
}