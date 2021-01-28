layout(binding = 0) SamplerState smSampler : register(s0);
layout(binding = 1) texture2D shadowMap : register(t0);
layout(binding = 2) texture2D shadowMapLinear : register(t1);
layout(binding = 3) texture2D worldPos : register(t2);
layout(binding = 4) texture2D worldNorm : register(t3);
layout(binding = 5) cbuffer LightVP : register(b0)
{
    float4x4 lightVP;
}
layout(binding = 6) cbuffer LightPos : register(b1)
{
    float4 lightPos;
}

/*
// Depth comparision in NDC space
float4 main(float2 texC : TEXCOORD) : SV_TARGET0
{
    float4 wPos = worldPos.Sample(smSampler, texC);
    wPos.w = 1.0;
    float4 cPosLight = mul(wPos, lightVP);
    cPosLight /= cPosLight.w;

    cPosLight.xy = cPosLight.xy * 0.5 + 0.5;
    cPosLight.y = 1 -  cPosLight.y;
    float depth = shadowMap.Sample(smSampler, cPosLight.xy).x;//shadowMap[10, 10].x);
    
    if (depth < cPosLight.z)
        return float4(0.0);

    return float4(1,1,1,1);

    //return float4(depth, cPosLight.z, cPosLight.x, cPosLight.y);
}*/


// Depth comparision in world space
float4 main(float2 texC : TEXCOORD) : SV_TARGET0
{
    float4 wPos = worldPos.Sample(smSampler, texC);
    float4 wNorm = worldNorm.Sample(smSampler, texC);

    wPos.w = 1.0;
    float4 cPosLight = mul(wPos, lightVP);
    cPosLight /= cPosLight.w;

    cPosLight.xy = cPosLight.xy * 0.5 + 0.5;
    cPosLight.y = 1 -  cPosLight.y;
    float depth = shadowMapLinear.Sample(smSampler, cPosLight.xy).x;//shadowMap[10, 10].x);

    float3 lightDir = lightPos.xyz - wPos.xyz;
    cPosLight.z = length(lightDir);
    lightDir /= cPosLight.z;

    //if (depth < cPosLight.z)
        //return float4(0.0);

    //return float4(1,1,1,1);

    float isValid = length(wPos.xyz) > 0.0001 ? 1.0 : 0.0;
    float skew = dot(lightDir, wNorm.xyz);
    skew = skew > 0.0 ? skew : 0.0; 
    return float4(depth * isValid, cPosLight.z, skew, cPosLight.y);
    //return float4(depth * isValid > cPosLight.z);
}
