import Scene.Raytracing;
import Utils.Sampling.SampleGenerator;

layout(binding = 0) SamplerState sampler : register(s0);
layout(binding = 1) texture2D worldPos : register(t0);
layout(binding = 2) texture2D worldNorm : register(t1);
layout(binding = 3) RWTexture2D outColor : register(t2);
layout(binding = 4) cbuffer LightData : register(b0)
{
    float4 lightData; // First three component indicate position (Point light) or direction (Directional Light), last component indicate point light (0) or directional light (1).
}
layout(binding = 5) cbuffer CB : register(b1)
{
    uint        gSeed;        // seed for PRNG
}

/** Payload for shadow ray.
*/
struct ShadowRayData
{
    bool visible;
};

[shader("miss")]
void shadowMiss(inout ShadowRayData rayData : SV_RayPayload)
{
    // The miss shader is executed if the ray misses all geometry. Mark as visible.
    rayData.visible = true;
}

[shader("closesthit")]
void shadowCHit(inout ShadowRayData rayData : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attribs : SV_IntersectionAttributes)
{
    // no-op
}

/** Traces a shadow ray towards a light source.
    \param[in] origin Ray origin for the shadow ray.
    \param[in] dir Direction from shading point towards the light source (normalized).
    \return True if light is visible, false otherwise.
*/
bool traceShadowRay(float3 origin, float3 dir)
{
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = dir;
    ray.TMin = 0.01f;
    ray.TMax = 1000.0f;

    ShadowRayData rayData;
    rayData.visible = false;    // Set to true by miss shader if ray is not terminated before
    TraceRay(gRtScene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xff /* instanceInclusionMask */, 0 /* hitIdx */, hitProgramCount, 0 /* missIdx */, ray, rayData);

    return rayData.visible;
}

/* No AA
[shader("raygeneration")]
void rayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
   
    float3 origin = worldPos[launchIndex].xyz;
    float3 lightDir =  lightData.w < 0.5 ? normalize(lightData.xyz - origin) : lightData.xyz;

    outColor[launchIndex] = float4(float3(traceShadowRay(origin, lightDir)), 1.0f);
}
*/

/* With AA, diffuse shading
[shader("raygeneration")]
void rayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchSize = DispatchRaysDimensions().xy;

    float3 outCol = float3(0);
    uint nSample = 4;
    float delta = 1.0 / nSample;
    float offset = delta / 2.0;

    for (uint i = 0; i < nSample; i++)
        for (uint j = 0; j < nSample; j++)
        {
            float2 texC = (float2(launchIndex) + float2(offset) + float2(i,j) * delta) / float2(launchSize);
            float3 origin = worldPos.SampleLevel(sampler, texC, 0).xyz; //float3 origin = worldPos[launchIndex].xyz;
            float3 normal = worldNorm.SampleLevel(sampler, texC, 0).xyz;

            float distance = lightData.w < 0.5 ? length(lightData.xyz - origin) : length(lightData.xyz);
            float3 lightDir =  lightData.w < 0.5 ? lightData.xyz - origin : lightData.xyz;

            lightDir /= distance;

            float cos = dot(lightDir, normal);
            cos = cos > 0 ? cos : 0;

            outCol += float3(traceShadowRay(origin, lightDir) * cos / (4 * distance * distance * 3.14159));
        }
    float lightPower = 600; // Change this in readExr->readFeature when changed here
     outColor[launchIndex] = float4(lightPower * outCol / (nSample * nSample), 1.0f);
}*/

/* With AA, diffuse shading
[shader("raygeneration")]
void rayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchSize = DispatchRaysDimensions().xy;

    float3 outCol = float3(0);
    uint nSample = 4;
    float delta = 1.0 / nSample;
    float offset = delta / 2.0;

    for (uint i = 0; i < nSample; i++)
        for (uint j = 0; j < nSample; j++)
        {
            float2 texC = (float2(launchIndex) + float2(offset) + float2(i,j) * delta) / float2(launchSize);
            float3 origin = worldPos.SampleLevel(sampler, texC, 0).xyz; //float3 origin = worldPos[launchIndex].xyz;
            float3 normal = worldNorm.SampleLevel(sampler, texC, 0).xyz;

            float distance = lightData.w < 0.5 ? length(lightData.xyz - origin) : length(lightData.xyz);
            float3 lightDir =  lightData.w < 0.5 ? lightData.xyz - origin : lightData.xyz;

            lightDir /= distance;

            float cos = dot(lightDir, normal);
            cos = cos > 0 ? cos : 0;

            outCol += float3(traceShadowRay(origin, lightDir) * cos / (4 * distance * distance * 3.14159));
        }
    float lightPower = 600; // Change this in readExr->readFeature when changed here
     outColor[launchIndex] = float4(lightPower * outCol / (nSample * nSample), 1.0f);
}*/

// soft-shadow
[shader("raygeneration")]
void rayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchSize = DispatchRaysDimensions().xy;

    SampleGenerator sg = SampleGenerator.create(launchIndex, gSeed);
    sampleNext1D(sg);
    sampleNext1D(sg);

    float3 outCol = float3(0);
    uint nSample = 10;
  
    float3 emitterNormal =  normalize(lightData.xyz);
    float3 emitterTangent = normalize(cross(emitterNormal, float3(1)));
    float3 emitterBiTangent = cross(emitterNormal, emitterTangent);
    float emitterSize = 1.0;// Note changing emitterSize here, also change emitterSize in SimpleSM::perturb 

    for (uint i = 0; i < nSample; i++) {
        float2 uv = sampleNext2D(sg);
        float2 texC = (float2(launchIndex) + uv) / float2(launchSize);
        float3 origin = worldPos.SampleLevel(sampler, texC, 0).xyz; //float3 origin = worldPos[launchIndex].xyz;
        float3 normal = worldNorm.SampleLevel(sampler, texC, 0).xyz;

        float3 emitterPosition = lightData.xyz + emitterSize * (uv.x - 0.5) * emitterTangent +  emitterSize * (uv.y - 0.5) * emitterBiTangent;
        float distance = length(emitterPosition - origin);
        float3 lightDir =  emitterPosition - origin;

        lightDir /= distance;

        float cos = dot(lightDir, normal);
        float cosEmitter = abs(dot(lightDir, emitterNormal));
        cos = cos > 0 ? cos : 0;

        outCol += float3(traceShadowRay(origin, lightDir) * cos * cosEmitter / (4 * distance * distance * 3.14159));
    }
    float lightPower = 1200; // Change this in readExr->readFeature when changed here
    outColor[launchIndex] = float4(lightPower * outCol / nSample, 1.0f);
}
