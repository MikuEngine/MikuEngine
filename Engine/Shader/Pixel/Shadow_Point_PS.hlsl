#include "../Include/Shared.hlsli"

float main(PS_INPUT_RT_INDEX input) : SV_Target
{
    float dist = distance(input.worldPos.xyz, g_shadowLightPosition);
    return dist / max(EPSILON, g_shadowLightRange);
}