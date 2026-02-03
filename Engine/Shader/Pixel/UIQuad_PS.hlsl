#include "../Include/Shared.hlsli"

// MaskMode
static const uint UI_MASK_NONE = 0u;
static const uint UI_MASK_RECT = 1u;
static const uint UI_MASK_CIRCLE = 2u;
static const uint UI_MASK_RING = 3u;
static const uint UI_MASK_RECTRING = 4u;
static const uint UI_MASK_RADIAL = 5u;

// EffectMode
static const uint UI_FX_NONE = 0u;
static const uint UI_FX_FLAMEBAR = 1u;
static const uint UI_FX_SHINESWEEP = 2u;
static const uint UI_FX_DISSOLVE = 3u;

bool InsideRect(float2 p, float4 r)
{
     return (p.x >= r.x && p.y >= r.y && p.x <= r.z && p.y <= r.w);
}

// Effect Util
float FillMask01(float u, float fill, float feather)
{
    return 1.0 - smoothstep(fill, fill + feather, u);
}

float HeadBand(float u, float head, float width)
{
    float d = abs(u - head);
    return 1.0 - smoothstep(width, width * 2.0, d);
}

float Noise01(float2 uv, float2 tiling, float2 speed, float time)
{
    float2 nuv = uv * tiling + speed * time;
    return g_texUINoise.Sample(g_samLinear, nuv).r;
}

float3 Ramp(float t)
{
    return g_texUIRamp.Sample(g_samLinear, float2(saturate(t), 0.5)).rgb;
}

//////////////////////////////////////////////////////////////////////////

// ex 1) 끝이 불타는 바
void ApplyFx_FlameBar(float2 uv, inout float4 col)
{
    float fill = saturate(g_uiMask1.y);

    float feather = g_effect0.x; // 0.005~0.02
    float headWidth = g_effect0.y; // 0.01~0.06

    float emissive = g_effect2.x; // 0~2
    float flameInt = g_effect2.y; // 0~2
    float alphaJit = g_effect2.z; // 0~0.3

    // Fill mask(부드럽게)
    float mFill = FillMask01(uv.x, fill, feather);
    col.a *= mFill;

    // Head band + noise
    float band = HeadBand(uv.x, fill, headWidth);
    float n = Noise01(uv, g_effect1.xy, g_effect1.zw, g_time);
    float flame = saturate(band * (0.4 + 0.6 * n));

    // Ramp color + additive glow
    float3 flameCol = Ramp(flame);
    col.rgb += flameCol * flame * flameInt * emissive;

    // optional alpha jitter
    col.a *= saturate(1.0 - alphaJit + alphaJit * n);
}

// ex 2) 반짝이는 띠가 지나감
void ApplyFx_ShineSweep(float2 uv, inout float4 col)
{
    float fill = saturate(g_uiMask1.y);

    float feather = g_effect0.x;
    float width = g_effect0.y; // 0.02~0.15
    float angle = g_effect0.z; // 라디안 (0=가로, 0.8=대각)
    float speed = g_effect1.z; // 스윕 속도

    float emissive = g_effect2.x;
    float intensity = g_effect2.y;

    // Fill mask도 함께 적용하고 싶으면 유지(프로그래스바용)
    float mFill = FillMask01(uv.x, fill, feather);
    col.a *= mFill;

    float2 dir = float2(cos(angle), sin(angle));
    float t = dot(uv, dir);

    float s = frac(t - g_time * speed);
    float band = 1.0 - smoothstep(width, width * 2.0, abs(s - 0.5));

    col.rgb += band * intensity * emissive;
}

// ex 3) 노이즈로 사라짐
void ApplyFx_Dissolve(float2 uv, inout float4 col)
{
    float thr = g_effect0.x; // 0..1
    float soft = g_effect0.y; // 0.01..0.2
    float edgeW = g_effect0.z; // 0.01..0.1

    float emissive = g_effect2.x;
    float edgeInt = g_effect2.y;

    float n = Noise01(uv, g_effect1.xy, g_effect1.zw, g_time);

    // alpha mask
    float a = smoothstep(thr, thr + soft, n);
    col.a *= a;

    // edge glow
    float edge = 1.0 - smoothstep(edgeW, edgeW * 2.0, abs(n - thr));
    float3 edgeCol = Ramp(edge);
    col.rgb += edgeCol * edge * edgeInt * emissive;
}

//////////////////////////////////////////////////////////////////////////



float4 main(PS_INPUT_TEXCOORD input) : SV_Target
{
    float2 p = input.position.xy;
    
    // Mask
    if (g_uiMaskMode == UI_MASK_RECT)
    {
        if (!InsideRect(p, g_uiClipRect))
            discard;
    }
    else if (g_uiMaskMode == UI_MASK_CIRCLE)
    {
        float2 c = g_uiMask0.xy;
        float r = g_uiMask0.z;
        
        if (distance(p,c) > r)
            discard;
    }
    else if (g_uiMaskMode == UI_MASK_RING)
    {
        // g_uiMask0 = (cx, cy, rInner, rOuter)
        float2 c = g_uiMask0.xy;
        float rInner = g_uiMask0.z;
        float rOuter = g_uiMask0.w;

        float d = distance(p, c);
        if (d < rInner || d > rOuter)
            discard;
    }
    else if (g_uiMaskMode == UI_MASK_RECTRING)
    {
        // outer = g_uiClipRect, inner = g_uiMask0 (xMin,yMin,xMax,yMax)
        bool insideOuter = InsideRect(p, g_uiClipRect);
        bool insideInner = InsideRect(p, g_uiMask0);

        if (!(insideOuter && !insideInner))
            discard;
    }
    else if (g_uiMaskMode == UI_MASK_RADIAL)
    {
        // g_uiMask0 = (cx, cy, rInner, rOuter)
        float2 c = g_uiMask0.xy;
        float rInner = g_uiMask0.z;
        float rOuter = g_uiMask0.w;
        
        // g_uiMask1 = (startAngleRad, fill01, clockwise(0/1), unused)
        float start = g_uiMask1.x;
        float fill = saturate(g_uiMask1.y);
        bool cw = (g_uiMask1.z > 0.5);
        
        float d = distance(p, c);
        if (d < rInner || d > rOuter)
            discard;
        
        float2 v = float2(p.x - c.x, c.y - p.y);

        float ang = atan2(v.y, v.x); // -pi~pi
        if (ang < 0)
            ang += 6.2831853; // 0~2pi

        // start도 0~2pi로 정규화(안전)
        if (start < 0)
            start += 6.2831853;
        start = fmod(start, 6.2831853);

        float span = fill * 6.2831853;

        float rel = ang - start;
        if (rel < 0)
            rel += 6.2831853;

        if (cw)
            rel = 6.2831853 - rel;

        if (rel > span)
            discard;
    }

    // Texture Sample
    float2 uv = input.texCoord;
    float4 tex = g_texBlit.Sample(g_samLinear, uv);
    float4 finalColor = tex * g_uiColor;
    
    // Effect branch
    switch (g_effectMode)
    {
        case UI_FX_FLAMEBAR:    ApplyFx_FlameBar(uv, finalColor);   break;  
        case UI_FX_SHINESWEEP:  ApplyFx_ShineSweep(uv, finalColor); break;
        case UI_FX_DISSOLVE:    ApplyFx_Dissolve(uv, finalColor);   break;
        default:  break;
    }
  
    finalColor.rgb = LinearToSRGB(finalColor.rgb);

    return finalColor;
}
