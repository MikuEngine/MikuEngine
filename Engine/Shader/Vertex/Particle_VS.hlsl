#include "../Include/Shared.hlsli"

static const float2 k_quadUVs[6] =
{
    float2(0.0f, 1.0f), // 0: Bottom-Left
    float2(0.0f, 0.0f), // 1: Top-Left
    float2(1.0f, 1.0f), // 2: Bottom-Right
    
    float2(0.0f, 0.0f), // 1: Top-Left
    float2(1.0f, 0.0f), // 3: Top-Right
    float2(1.0f, 1.0f) // 2: Bottom-Right
};
static const float2 k_quadOffsets[6] =
{
    float2(-0.5f, -0.5f),
    float2(-0.5f, 0.5f),
    float2(0.5f, -0.5f),
    
    float2(-0.5f, 0.5f),
    float2(0.5f, 0.5f),
    float2(0.5f, -0.5f)
};

PS_INPUT_PARTICLE main(uint vertexID : SV_VertexID)
{
    PS_INPUT_PARTICLE output;
    
    uint particleIndex = vertexID / 6; // 몇 번째 파티클인지
    uint vertexIndex = vertexID % 6; // 사각형의 몇 번째 점인지
    
    PARTICLE_DATA p = g_sbParticleData[particleIndex];
    
    
    float3 right = normalize(g_invView[0].xyz);
    float3 up = normalize(g_invView[1].xyz);
    
    float rad = radians(p.rotation);
    float c = cos(rad);
    float s = sin(rad);
    float2 offset = k_quadOffsets[vertexIndex];
    
    // 2D 회전 적용
    float2 rotOffset;
    rotOffset.x = offset.x * c - offset.y * s;
    rotOffset.y = offset.x * s + offset.y * c;
    float halfSize = p.size * 0.5f;
    float3 finalPos = p.position 
        + right * (rotOffset.x * halfSize)
        + up * (rotOffset.y * halfSize);
    
    // 4. 출력 설정
    output.position = mul(float4(finalPos, 1.0f), g_viewProjection);
    output.color = p.color;
    output.texCoord = k_quadUVs[vertexIndex] * p.uvScale + p.uvOffset;
    
    return output;
}