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
#include "WireframePass.h"

// Don't remove this. it's required for hot-reload to function properly
extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerClass("WireframePass", "Render Pass Template", WireframePass::create);
}

WireframePass::SharedPtr WireframePass::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new WireframePass);
    return pPass;
}

Dictionary WireframePass::getScriptingDictionary()
{
    return Dictionary();
}

RenderPassReflection WireframePass::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    //reflector.addInput("input", "the source texture");
    reflector.addOutput("output", "the destination texture");
    return reflector;
}

void WireframePass::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    auto pTargetFbo = Fbo::create({ renderData["output"]->asTexture() });
    const float4 clearColor(0, 0, 0, 1);
    pRenderContext->clearFbo(pTargetFbo.get(), clearColor, 1.0f, 0, FboAttachmentType::All);
    mpGraphicsState->setFbo(pTargetFbo);

    // Set render state
    Scene::RenderFlags renderFlags = Scene::RenderFlags::UserRasterizerState;
    if (mpVars != nullptr)
        mpVars["PerFrameCB"]["gColor"] = float4(0, 1, 0, 1);

    if (mpScene != nullptr)
        mpScene->render(pRenderContext, mpGraphicsState.get(), mpVars.get(), renderFlags);
}

void WireframePass::renderUI(Gui::Widgets& widget)
{
}

void WireframePass::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    mpProgram->addDefines(mpScene->getSceneDefines());
    mpVars = GraphicsVars::create(mpProgram->getReflector());
}

WireframePass::WireframePass()
{
    mpProgram = GraphicsProgram::createFromFile("RenderPasses/WireframePass/Wireframe.ps.slang", "", "main");

    RasterizerState::Desc wireframeDesc;
    wireframeDesc.setFillMode(RasterizerState::FillMode::Wireframe);
    wireframeDesc.setCullMode(RasterizerState::CullMode::None);
    mpRasterState = RasterizerState::create(wireframeDesc);

    mpGraphicsState = GraphicsState::create();
    mpGraphicsState->setProgram(mpProgram);
    mpGraphicsState->setRasterizerState(mpRasterState);
}
