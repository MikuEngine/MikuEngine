#include "../Include/Shared.hlsli"

static const uint UI_MASK_NONE = 0u;
static const uint UI_MASK_RECT = 1u;
static const uint UI_MASK_CIRCLE = 2u;
static const uint UI_MASK_RING = 3u;
static const uint UI_MASK_RECTRING = 4u;
static const uint UI_MASK_RADIAL = 5u;

bool InsideRect(float2 p, float4 r)
{
     return (p.x >= r.x && p.y >= r.y && p.x <= r.z && p.y <= r.w);
}

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
   
    finalColor.rgb = LinearToSRGB(finalColor.rgb);

    return finalColor;
}
