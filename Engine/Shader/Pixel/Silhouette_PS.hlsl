#include "../Include/Shared.hlsli"

// 잔상 2패스 실루엣용: Material CB만 사용. RGB에 materialEmissiveIntensity 적용 (0이면 1로, 1 이상이면 더 밝게)
float4 main(PS_INPUT_TEXCOORD input) : SV_Target
{
    float scale = g_materialEmissiveIntensity > 0.0 ? g_materialEmissiveIntensity : 1.0;
    float3 rgb = g_materialBaseColor.rgb * scale;
    float a = g_materialBaseColor.a * g_materialAlpha;
    return float4(rgb * a, a);
}
