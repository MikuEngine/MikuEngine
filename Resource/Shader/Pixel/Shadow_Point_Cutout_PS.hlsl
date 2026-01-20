#include "../Include/Shared.hlsli"

float main(PS_INPUT_RT_INDEX input) : SV_Target
{
    float threshold = 0.001f;
    
    clip(g_texBaseColor.Sample(g_samLinear, input.texCoord).a - threshold);
    
    float dist = distance(input.worldPos.xyz, g_shadowLightPosition);
    return dist / max(EPSILON, g_shadowLightRange);
}