#include "../Include/Shared.hlsli"

#define MAX_OUTLINE_RADIUS 4

static const uint UI_MASK_RECT = 1u;

bool InsideRect(float2 p, float4 r)
{
    return (p.x >= r.x && p.y >= r.y && p.x <= r.z && p.y <= r.w);
}

// 알파 샘플 (UV 범위 밖은 0으로 처리)
float AlphaAt(float2 uv)
{
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
        return 0.0;

    return g_texBlit.Sample(g_samLinear, uv).a;
}

float4 main(PS_INPUT_TEXCOORD input) : SV_Target
{
    float2 uv = input.texCoord;

    float2 p = input.position.xy;
    
    // Mask
    if (g_uiMaskMode == UI_MASK_RECT)
    {
        if (!InsideRect(p, g_uiClipRect))
            discard;
    }
    
    // 중심 픽셀 알파
    float centerAlpha = AlphaAt(uv);

    // 원본 픽셀은 그대로 출력하지 않음 (Outline 패스 전용)
    if (centerAlpha > 0.0)
        discard;

    // texel 크기 계산
    uint w, h;
    g_texBlit.GetDimensions(w, h);
    float2 texel = float2(1.0 / max(1u, w), 1.0 / max(1u, h));

    bool hasNeighbor = false;
    
    // 주변 픽셀 탐색 (원형)
    [unroll]
    for (int y = -MAX_OUTLINE_RADIUS; y <= MAX_OUTLINE_RADIUS; ++y)
    {
        [unroll]
        for (int x = -MAX_OUTLINE_RADIUS; x <= MAX_OUTLINE_RADIUS; ++x)
        {
            if (x == 0 && y == 0)
                continue;

            if ((x * x + y * y) > (MAX_OUTLINE_RADIUS * MAX_OUTLINE_RADIUS))
                continue;

            float2 suv = uv + float2((float) x, (float) y) * texel;

            if (AlphaAt(suv) > 0.0)
            {
                hasNeighbor = true;
                break;
            }
        }

        if (hasNeighbor)
            break;
    }

    // 주변에 원본 픽셀이 있으면 → 아웃라인
    if (hasNeighbor)
    {
        return g_outlineColor;
    }

    discard;
    
    return float4(0, 0, 0, 1);
}
