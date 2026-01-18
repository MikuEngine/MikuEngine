#include "../Include/Shared.hlsli"

static const uint UI_MASK_NONE = 0u;
static const uint UI_MASK_RECT = 1u;
static const uint UI_MASK_CIRCLE = 2u;
static const uint UI_MASK_RING = 3u;
static const uint UI_MASK_RECTRING = 4u;

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

    float4 tex = g_texBlit.Sample(g_samLinear, input.texCoord);
    return tex * g_uiColor;
}
