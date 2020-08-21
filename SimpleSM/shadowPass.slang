import Scene.Raster;

cbuffer LightVP
{
    float4x4 lightVP;
}

float4 vsMain(VSIn vIn) : SV_POSITION
{
    float4x4 worldMat = gScene.getWorldMatrix(vIn.meshInstanceID);
    //float4x4 viewProjMat = gScene.camera.getViewProj();
    float4x4 mvpMat = mul(worldMat, lightVP);
    return mul(float4(vIn.pos, 1.f), mvpMat);
}

void psMain(float4 posW : SV_POSITION)
{
    
}

/*
//For debugging
float psMain(float4 posW : SV_POSITION) : SV_DEPTH
{
    return lightVP[0][0];
}
*/
