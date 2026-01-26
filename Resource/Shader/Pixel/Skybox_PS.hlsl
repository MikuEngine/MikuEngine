#include "../Include/Shared.hlsli"

float4 main(PS_INPUT_LOCAL_POSITION input) : SV_Target
{
    if (g_useSkyboxTexture > 0.5f)
    {
        // 텍스처 사용
        return float4(g_texIBLEnvironment.Sample(g_samLinear, input.localPosition).rgb, 1.0f);
    }
    else
    {
        // 색상 사용 (그라데이션: 위는 skyboxColor, 아래는 skyboxHorizonColor)
        float3 direction = normalize(input.localPosition);
        float t = (direction.y + 1.0f) * 0.5f; // -1~1을 0~1로 변환
        float3 color = lerp(g_skyboxHorizonColor, g_skyboxColor, t);
        return float4(color, 1.0f);
    }
}