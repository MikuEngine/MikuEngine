#include "../Include/Shared.hlsli"

// 잔상 솔리드 레이어용: 빛 영향 없이 emissive만 출력 (언릿)
// RGB에 alpha 곱해 프리멀티플라이드 출력 → 블렌드 One, InvSrcAlpha 사용
float4 main(PS_INPUT_GBUFFER input, bool isFrontFace : SV_IsFrontFace) : SV_Target
{
    float2 uv = input.texCoord;
    float4 baseColor = g_texBaseColor.Sample(g_samLinear, uv);
    baseColor.a *= g_materialAlpha;
    float3 emissive = g_materialEmissive * g_materialEmissiveIntensity;
    return float4(emissive * baseColor.a, baseColor.a);
}
