#include "../Include/Shared.hlsli"
#include "../Include/PBR_Shared.hlsli"

// Spot Light 계산을 위한 픽셀 셰이더
// 1. G-Buffer에서 데이터 복원
// 2. 조명 위치와 픽셀 위치로 L 벡터 계산
// 3. 거리 감쇠 (Distance Attenuation) 계산
// 4. 각도 감쇠 (Angular Attenuation - Spot Factor) 계산
// 5. PBR BRDF 계산
float4 main(PS_INPUT input) : SV_Target
{
    float2 uv = input.position.xy / g_screenSize;
    float depth = g_gBufferDepth.Sample(g_samPoint, uv).r;
    
    // 깊이값이 1.0(초기화 값)이면 렌더링할 물체가 없는 것이므로 조명 연산 스킵
    if (depth >= 1.0f)
    {
        discard;
    }
    
    // Lighting Flag 확인 (0.1 미만이면 Unlit 등으로 간주하여 조명 연산 제외)
    float4 encodedNormal = g_gBufferNormal.Sample(g_samPoint, uv).rgba;
    float lightingFlag = encodedNormal.a;
    if (lightingFlag < 0.1f)
    {
        discard;
    }
    
    // G-Buffer에서 데이터 샘플링
    float3 baseColor = g_gBufferBaseColor.Sample(g_samPoint, uv).rgb;
    float3 orm = g_gBufferORM.Sample(g_samPoint, uv).rgb;
    float ao = orm.r;
    float roughness = orm.g;
    float metalness = orm.b;
    
    // World Position 복원
    float4 clipPosition;
    clipPosition.x = uv.x * 2.0f - 1.0f;
    clipPosition.y = -(uv.y * 2.0f - 1.0f);
    clipPosition.z = depth;
    clipPosition.w = 1.0f;
    
    float4 worldPosition = mul(clipPosition, g_invViewProjection);
    worldPosition /= worldPosition.w;
    
    // Normal 복원
    float3 n = normalize(DecodeNormal(encodedNormal.rgb));
    
    // View Vector
    float3 v = normalize(g_cameraWorldPosition - worldPosition.xyz);
    
    // Light Vector (표면 -> 조명)
    float3 lVec = g_lightPosition - worldPosition.xyz;
    float dist = length(lVec);
    
    // 라이트 범위 밖이면 폐기
    if (dist > g_lightRange)
    {
        discard;
    }
    
    float3 l = lVec / max(dist, EPSILON); // Normalize
    
    // N dot L (조명을 받지 않는 뒷면 제외)
    float nDotL = saturate(dot(n, l));
    if (nDotL <= 0.0f)
    {
        discard;
    }
    
    // --- Attenuation Calculation ---
    
    // 1. Distance Attenuation (거리 감쇠)
    // 거리가 멀어질수록 어두워짐. (point light와 동일)
    float distAtt = saturate(1.0f - (dist / g_lightRange));
    distAtt *= distAtt; // 제곱 감쇠로 좀 더 자연스럽게
    
    // 2. Angular Attenuation (각도 감쇠 - Spot Factor)
    // 조명의 방향(g_lightDirection)과 표면에서 조명으로 향하는 벡터(-l) 사이의 각도 계산
    // g_lightDirection은 조명이 비추는 방향이므로, 벡터의 내적을 위해 -l과 비교하거나, 
    // 혹은 pixel -> light 벡터인 l과, light -> pixel 방향인 g_lightDirection을 비교할 때는 -를 붙여야 함.
    // 여기서는 l(Surface to Light)을 사용하므로, dot(-l, g_lightDirection)이 코사인 값임.
    
    float cosAngle = dot(-l, g_lightDirection);
    
    // 반각(Half-Angle)을 라디안으로 변환
    float outerAngleRad = radians(g_lightAngle);
    float innerAngleRad = outerAngleRad * 0.8f; // 내부 각도는 외부의 80% 정도로 설정하여 부드럽게 감쇠 (필요시 조절 가능)
    
    float cosOuter = cos(outerAngleRad);
    float cosInner = cos(innerAngleRad);
    
    // Smoothstep을 사용하여 inner와 outer 사이를 부드럽게 보간
    // cosAngle이 cosOuter보다 작으면(각도가 더 넓으면) 0, cosInner보다 크면 1
    float spotAtt = smoothstep(cosOuter, cosInner, cosAngle);
    
    float attenuation = distAtt * spotAtt;
    
    if (attenuation <= 0.0f)
    {
        discard;
    }

    // --- PBR Calculation ---
    
    float3 h = normalize(l + v);
    float nDotH = saturate(dot(n, h));
    float nDotV = saturate(dot(n, v));
    float hDotV = saturate(dot(h, v));
    
    float3 f0 = lerp(DielectricFactor, baseColor.rgb, metalness);
    
    float g = GAFSchlickGGX(nDotV, nDotL, roughness);
    float d = NDFGGXTR(nDotH, max(0.04f, roughness));
    float3 f = FresnelSchlick(f0, hDotV);
    
    float3 kd = lerp(1.0f - f, 0.0f, metalness);
    
    float3 diffuseBRDF = kd * baseColor.rgb / PI;
    float3 specularBRDF = (f * d * g) / max(0.0001f, 4.0f * nDotL * nDotV);
    
    float3 final = (diffuseBRDF + specularBRDF) * g_lightColor * g_lightIntensity * nDotL * attenuation;
    
    return float4(final, 1.0f);
}