import Scene.Raytracing;

layout(binding = 0) SamplerState sampler : register(s0);
layout(binding = 1) texture2D worldPos : register(t0);
layout(binding = 2) RWTexture2D outColor : register(t1);
layout(binding = 3) cbuffer LightData : register(b0)
{
    float4 lightData; // First three component indicate position (Point light) or direction (Directional Light), last component indicate point light (0) or directional light (1).
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

// With AA
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
    
            float3 lightDir =  lightData.w < 0.5 ? normalize(lightData.xyz - origin) : lightData.xyz;

            outCol += float3(traceShadowRay(origin, lightDir));
        }

     outColor[launchIndex] = float4(outCol / (nSample * nSample), 1.0f);
}
