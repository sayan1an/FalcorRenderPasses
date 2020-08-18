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
#include "SimpleSM.h"

// Don't remove this. it's required for hot-reload to function properly
extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerClass("SimpleSM", "Render Pass Template", SimpleSM::create);
}

SimpleSM::SharedPtr SimpleSM::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new SimpleSM);
    return pPass;
}

Dictionary SimpleSM::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection SimpleSM::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addOutput("output", "Destination texture");
    //reflector.addInput("src");
    return reflector;
}

void SimpleSM::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    const float4 clearColor(0, 0, 0, 1);
    pRenderContext->clearFbo(mShadowPass.pFbo.get(), clearColor, 1.0f, 0, FboAttachmentType::Depth);
  
    if (mpScene != nullptr)
        mpScene->render(pRenderContext, mShadowPass.mpGraphicsState.get(), mShadowPass.mpVars.get());

    mVisibilityPass.pFbo->attachColorTarget(renderData["output"]->asTexture(), 0);
    pRenderContext->clearFbo(mVisibilityPass.pFbo.get(), clearColor, 1.0f, 0, FboAttachmentType::Color);
    mVisibilityPass.pPass->execute(pRenderContext, mVisibilityPass.pFbo);
}

void SimpleSM::renderUI(Gui::Widgets& widget)
{
}

void SimpleSM::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    mShadowPass.mpProgram->addDefines(mpScene->getSceneDefines());
    mShadowPass.mpVars = GraphicsVars::create(mShadowPass.mpProgram->getReflector());
}

void SimpleSM::ShadowPass::resetDepthTexture()
{
    pDepth = Texture::create2D(width, height, ResourceFormat::D32Float, 1, 1, nullptr, Resource::BindFlags::DepthStencil | Resource::BindFlags::ShaderResource);
    pFbo->attachDepthStencilTarget(pDepth);
    mpGraphicsState->setFbo(pFbo);

    GraphicsState::Viewport VP;
    VP.originX = 0;
    VP.originY = 0;
    VP.minDepth = 0;
    VP.maxDepth = 1;
    VP.height = static_cast<float>(height);
    VP.width = static_cast<float>(width);

    mpGraphicsState->setViewport(0, VP);
}

SimpleSM::SimpleSM()
{
    GraphicsProgram::Desc desc;
    desc.addShaderLibrary("RenderPasses/SimpleSM/shadowPass.slang");
    desc.vsEntry("vsMain").psEntry("psMain");
    mShadowPass.mpProgram = GraphicsProgram::create(desc);
    mShadowPass.mpGraphicsState = GraphicsState::create();
    mShadowPass.mpGraphicsState->setProgram(mShadowPass.mpProgram);
    mShadowPass.pFbo = Fbo::create();
    mShadowPass.resetDepthTexture();
   
    mVisibilityPass.pPass = FullScreenPass::create("RenderPasses/SimpleSM/visibilityPass.ps.slang");
    mVisibilityPass.mpVars = mVisibilityPass.pPass->getVars();
    mVisibilityPass.pFbo = Fbo::create();

    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Point, Sampler::Filter::Point, Sampler::Filter::Point).setAddressingMode(Sampler::AddressMode::Border, Sampler::AddressMode::Border, Sampler::AddressMode::Border);
    samplerDesc.setLodParams(0.f, 0.f, 0.f);
    samplerDesc.setComparisonMode(Sampler::ComparisonMode::Disabled);

    mVisibilityPass.mpVars["smSampler"] = Sampler::create(samplerDesc);
    mVisibilityPass.mpVars["shadowMap"] = mShadowPass.pDepth;
}
