import Scene.Raster;

cbuffer LightVP
{
    float4x4 lightVP;
}

cbuffer LightPos
{
    float4 lightPos;
}

struct VsOut
{
    float4 worldPos : POSITION;
    float4 worldPosPerspective : SV_POSITION;
}

VsOut vsMain(VSIn vIn)
{
    VsOut vsOut;
    float4x4 worldMat = gScene.getWorldMatrix(vIn.meshInstanceID);
    //float4x4 viewProjMat = gScene.camera.getViewProj();
    float4x4 mvpMat = mul(worldMat, lightVP);

    vsOut.worldPos = mul(float4(vIn.pos, 1.f), worldMat);
    vsOut.worldPosPerspective = mul(float4(vIn.pos, 1.f), mvpMat);

    return vsOut;
}

float psMain(VsOut vsOut) : SV_TARGET0
{
    return length(vsOut.worldPos.xyz - lightPos.xyz);
}

/*
//For debugging
float psMain(float4 posW : SV_POSITION) : SV_DEPTH
{
    return lightVP[0][0];
}
*/
