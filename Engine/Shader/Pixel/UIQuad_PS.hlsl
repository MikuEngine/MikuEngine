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

static const uint UI_FX_SCANLINE         = 1u;
static const uint UI_FX_GLOW_PULSE       = 2u; 


static const uint UI_FX_PIXELATE         = 3u; //



static const uint UI_FX_HOVER_TRANSITION = 4u;

static const uint UI_FX_ABYSSAL_DECAY    = 5u;
static const uint UI_FX_STATIC_NOISE     = 6u;



//static const uint UI_FX_WAVE_DISTORT     = 5u;

// OnlyProgressBar
static const uint UI_FX_FLAMEBAR = 10u;
static const uint UI_FX_PURPLECURSE = 11u;

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

float2 Rotate2D(float2 v, float a)
{
    float s = sin(a), c = cos(a);
    return float2(c * v.x - s * v.y, s * v.x + c * v.y);
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

// ex 5) 스캔라인 (움직이는 가로줄 효과)
void ApplyFx_Scanline(float2 uv, inout float4 col)
{
    float density = g_effect0.x;
    float speed = g_effect0.y;
    float opacity = g_effect0.z;
    
    float scan = sin(uv.y * density + g_time * speed);
    scan = smoothstep(0.5, 1.0, scan);
    
    col.rgb += scan * opacity * col.rgb;
}

// ex 6) 글로우 펄스
void ApplyFx_GlowPulse(inout float4 col)
{
    float speed = g_effect0.x;
    float minI = g_effect0.y;
    float maxI = g_effect0.z;
    
    float pulse = (sin(g_time * speed) * 0.5 + 0.5);
    float intensity = lerp(minI, maxI, pulse);
    
    //col.rgb *= intensity;
    col.rgb += intensity * 0.5f;
}

// ex 7) 픽셀화
void ApplyFx_Pixelate(inout float2 uv)
{
    float pixelSize = g_effect0.x;
    if (pixelSize > 1.0)
    {
        // UV를 특정 그리드 단위로 끊어서 계단 현상을 만듭니다.
        uv.x = floor(uv.x * pixelSize) / pixelSize;
        uv.y = floor(uv.y * pixelSize) / pixelSize;
    }
}

// ex 8) 호버 애니메이션 (픽셀화 -> 스캔라인 -> 글로우 전환)
void ApplyFx_HoverTransition(float2 uv, inout float4 col)
{
    // g_effect1.x를 '시작 시간(startTime)'으로 활용합니다.
    float startTime = g_effect1.x;
    float localTime = g_time - startTime; // 애니메이션 진행 시간
    
    // 1. 초반 0.3초: 강한 픽셀화 (선명해지는 연출)
    if (localTime < 0.3)
    {
        float t = localTime / 0.3;
        // 16px -> 512px(원본수준)로 선명해짐
        float pSize = lerp(16.0, 512.0, t);
        uv.x = floor(uv.x * pSize) / pSize;
        uv.y = floor(uv.y * pSize) / pSize;
    }

    // 2. 0.1초 ~ 0.5초: 스캔라인이 스쳐 지나감
    if (localTime > 0.1 && localTime < 0.5)
    {
        float scan = sin(uv.y * 100.0 + localTime * 50.0);
        col.rgb += smoothstep(0.8, 1.0, scan) * 0.3;
    }

    // 3. 0.2초 이후 ~ 계속: 은은한 글로우 펄스
    if (localTime > 0.2)
    {
        float pulse = (sin(g_time * 3.0) * 0.5 + 0.5);
        // 흰색 버튼을 고려한 Additive Glow
        col.rgb += pulse * float3(0.2, 0.2, 0.2);
    }
}

void ApplyFx_LiquidShine(float2 uv, inout float4 col)
{
    // 대각선 방향 계산
    float angle = uv.x + uv.y;
    // 시간에 따라 흐르는 띠 (0~1 반복)
    float shine = frac(angle * 0.5 - g_time * 0.8);
    
    // 띠를 얇고 선명하게 만듦
    shine = smoothstep(0.4, 0.5, shine) - smoothstep(0.5, 0.6, shine);
    
    // 흰색 광원 추가 (Additive)
    col.rgb += shine * 0.6;
}

void ApplyFx_AbyssalDecay(float2 uv, inout float4 col)
{
    float startTime = g_effect1.x;
    float localTime = g_time - startTime;

    // 시작 전이라면 효과 미적용
    if (localTime < 0.0)
        return;

    // 0에서 1초까지 흐르게 설정 (애니메이션 속도 조절)
    float intensity = saturate(localTime / 1.0f);
    
    float2 center = uv - 0.5;
    float dist = dot(center, center) * 2.0;

    // 노이즈 오프셋에도 localTime을 써서 움직이게 만듦
    float n = Noise01(uv, float2(4.0, 4.0), float2(0.1, 0.1), localTime * 0.2);

    // [핵심 수정] intensity를 곱하거나 dist에서 빼주는 방식으로 '확장' 연출
    // intensity가 0일 때는 mask가 거의 0이 되도록 설정
    float mask = smoothstep(0.4, 0.9, (dist + n * 0.3) * intensity);

    // 외곽을 검게 태움
    col.rgb = lerp(col.rgb, float3(0, 0, 0), mask);
    col.rgb *= (1.0 - mask * 0.5);
}

void ApplyFx_StaticNoise(float2 uv, inout float4 col)
{
    // 아주 빠른 무작위 입자 노이즈
    float noise = frac(sin(dot(uv + g_time, float2(12.9898, 78.233))) * 43758.5453);
    
    // 원본 이미지에 노이즈를 얇게 덮음
    float noiseIntensity = 0.15;
    col.rgb = lerp(col.rgb, float3(noise, noise, noise), noiseIntensity);
    
    // 가끔 화면이 위아래로 튀는 효과 (Jitter)
    float jump = step(0.98, frac(sin(g_time) * 123.45)) * 0.01;
    uv.y += jump;
}

// OnlyProgressBar
void ApplyFx_PurpleCurse(float2 uv, inout float4 col)
{
    float fillProgress = saturate(g_uiMask1.y);
    float intensity = g_effect0.x; // 현재 1.0 ~ 10.0 사이의 값

    // 노이즈 속도에도 intensity를 섞어보세요. 강할수록 빨리 꿈틀거립니다.
    float n = Noise01(uv, float2(4.0, 4.0), float2(0.1, 0.5), g_time * (0.5 + intensity * 0.1));

    // 핵심: intensity를 '지수'로 사용하거나 '대비'로 사용
    // n이 0.5 근처일 때 intensity가 높을수록 0 또는 1로 확 쏠리게 만듭니다.
    float contrastN = saturate((n - 0.5) * intensity + 0.5);

    float corruption = smoothstep(1.2 - fillProgress, 1.5 - fillProgress, contrastN);
    
    // 광원 밝기 조절
    float3 basePurple = float3(0.5, 0.0, 1.0);
    float3 curseColor = basePurple * contrastN * fillProgress * (intensity * 0.5);
    
    col.rgb = lerp(col.rgb, float3(0, 0, 0), corruption);
    col.rgb += curseColor;
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
    
    // UV Base Effect
    if (g_effectMode == UI_FX_PIXELATE)
    {
        ApplyFx_Pixelate(uv);
    }
    if (g_effectMode == UI_FX_HOVER_TRANSITION)
    {
        float4 dummyCol = float4(0, 0, 0, 0);
        ApplyFx_HoverTransition(uv, dummyCol);
    }
    
    float4 tex = g_texBlit.Sample(g_samLinear, uv);
    float4 finalColor = tex * g_uiColor;

    // 3) Color/Alpha FX
    switch (g_effectMode)
    {
        case UI_FX_SCANLINE:
            ApplyFx_Scanline(uv, finalColor);
            break;
        case UI_FX_GLOW_PULSE:
            ApplyFx_GlowPulse(finalColor);
            break;
        
        case UI_FX_ABYSSAL_DECAY:
            ApplyFx_PurpleCurse(uv, finalColor);
            break;
        case UI_FX_STATIC_NOISE:
            ApplyFx_StaticNoise(uv, finalColor);
            break;
        
        case UI_FX_FLAMEBAR:
            ApplyFx_FlameBar(uv, finalColor);
            break;
        case UI_FX_PURPLECURSE:
            ApplyFx_PurpleCurse(uv, finalColor);
            break;
        
        default:
            break;
    }

    finalColor.rgb = LinearToSRGB(finalColor.rgb);
    return finalColor;
}
