/***************************************************************************
 # Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions
 # are met:
 #  * Redistributions of source code must retain the above copyright
 #    notice, this list of conditions and the following disclaimer.
 #  * Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in the
 #    documentation and/or other materials provided with the distribution.
 #  * Neither the name of NVIDIA CORPORATION nor the names of its
 #    contributors may be used to endorse or promote products derived
 #    from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 # EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 # PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 # OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************/
#include "PointShadowRT.h"

// Don't remove this. it's required for hot-reload to function properly
extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerClass("PointShadowRT", "Raytraced shadow map for point lights", PointShadowRT::create);
}

PointShadowRT::SharedPtr PointShadowRT::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new PointShadowRT);
    return pPass;
}

Dictionary PointShadowRT::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection PointShadowRT::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addOutput("output", "Shadow Map").bindFlags(ResourceBindFlags::UnorderedAccess).format(ResourceFormat::RGBA32Float);
    reflector.addInput("worldPos", "World Position");
    reflector.addInput("worldNorm", "World Normal");
    return reflector;
}

static float4 getLightData(const Light* pLight)
{
    // First three component indicate position (Point light) or direction (Directional Light), last component indicate point light (0) or directional light (1).
    float4 data; 
    switch (pLight->getType()) {
    case LightType::Directional:
        data = float4(static_cast<const DirectionalLight*>(pLight)->getWorldDirection(), 1.0f);
        break;
    case LightType::Point:
        data = float4(static_cast<const PointLight*>(pLight)->getWorldPosition(), 0.0f);
        break;
    default:
        should_not_get_here(); 
    }

    return data;
}
void PointShadowRT::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    mVisibilityPass.mpVars["worldPos"] = renderData["worldPos"]->asTexture();
    mVisibilityPass.mpVars["worldNorm"] = renderData["worldNorm"]->asTexture();
    mVisibilityPass.mpVars["outColor"] = renderData["output"]->asTexture();
    mVisibilityPass.mpVars["LightData"]["lightData"] = getLightData(mpScene->getLight(0).get());

    const uint2 targetDim = renderData.getDefaultTextureDims();
    assert(targetDim.x > 0 && targetDim.y > 0);

    // calls ray-gen
    mpScene->raytrace(pRenderContext, mVisibilityPass.mpProgram.get(), mVisibilityPass.mpVars, uint3(targetDim, 1));
}

void PointShadowRT::renderUI(Gui::Widgets& widget)
{
}

void PointShadowRT::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    mVisibilityPass.mpProgram->addDefines(mpScene->getSceneDefines());
    mVisibilityPass.mpVars = RtProgramVars::create(mVisibilityPass.mpProgram, mpScene);

    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Point).setAddressingMode(Sampler::AddressMode::Border, Sampler::AddressMode::Border, Sampler::AddressMode::Border);
    samplerDesc.setLodParams(0.f, 0.f, 0.f);
    samplerDesc.setComparisonMode(Sampler::ComparisonMode::Disabled);

    mVisibilityPass.mpVars["sampler"] = Sampler::create(samplerDesc);
}

PointShadowRT::PointShadowRT()
{
    RtProgram::Desc progDesc;
    progDesc.addShaderLibrary("RenderPasses/PointShadowRT/shadow.rt.slang").setRayGen("rayGen");
    progDesc.addMiss(0, "shadowMiss");
    progDesc.addHitGroup(0, "shadowCHit"); // A no-op hit-group must be provided, otherwise the program crashes.
    progDesc.setMaxTraceRecursionDepth(1);
    mVisibilityPass.mpProgram = RtProgram::create(progDesc, 4, 8); // 4 bytes - size of ray-payload, Default 8 bytes size of intersection/hit info (for builtin struct BuiltInTriangleIntersectionAttributes)
}
