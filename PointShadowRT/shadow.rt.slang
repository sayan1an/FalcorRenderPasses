import Scene.Raytracing;

layout(binding = 0) texture2D worldPos : register(t0);
layout(binding = 1) RWTexture2D outColor : register(t1);
layout(binding = 2) cbuffer LightData : register(b0)
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

[shader("raygeneration")]
void rayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
   
    float3 origin = worldPos[launchIndex].xyz;
    float3 lightDir =  lightData.w < 0.5 ? normalize(lightData.xyz - origin) : lightData.xyz;

    outColor[launchIndex] = float4(float3(traceShadowRay(origin, lightDir)), 1.0f);
}

