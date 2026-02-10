#include "../Include/Shared.hlsli"

float4 main(PS_INPUT_LOCAL_POSITION input) : SV_Target
{
    return float4(g_texIBLEnvironment.Sample(g_samLinear, input.localPosition).rgb * g_iblIntensity, 1.0f);
}