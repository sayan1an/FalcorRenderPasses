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

static void createShadowMatrix(const DirectionalLight* pLight, const float3& center, float radius, glm::mat4& shadowVP)
{
    glm::mat4 view = glm::lookAt(center, center + pLight->getWorldDirection(), float3(0, 1, 0));
    glm::mat4 proj = glm::ortho(-radius, radius, -radius, radius, -radius, radius);

    shadowVP = proj * view;
}

static void createShadowMatrix(const PointLight* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& shadowVP)
{
    const float3 lightPos = pLight->getWorldPosition();
    const float3 lookat = pLight->getWorldDirection() + lightPos;
    float3 up(0, 1, 0);
    if (abs(glm::dot(up, pLight->getWorldDirection())) >= 0.95f)
    {
        up = float3(1, 0, 0);
    }

    glm::mat4 view = glm::lookAt(lightPos, lookat, up);
    float distFromCenter = glm::length(lightPos - center);
    float nearZ = std::max(0.1f, distFromCenter - radius);
    float maxZ = std::min(radius * 2, distFromCenter + radius);
    float angle = pLight->getOpeningAngle() * 2;
    glm::mat4 proj = glm::perspective(angle, fboAspectRatio, nearZ, maxZ);

    shadowVP = proj * view;
}

static void createShadowMatrix(const Light* pLight, const float3& center, float radius, float fboAspectRatio, glm::mat4& shadowVP)
{
    switch (pLight->getType())
    {
    case LightType::Directional:
        return createShadowMatrix((DirectionalLight*)pLight, center, radius, shadowVP);
    case LightType::Point:
        return createShadowMatrix((PointLight*)pLight, center, radius, fboAspectRatio, shadowVP);
    default:
        should_not_get_here();
    }
}

static void getLightPosition(const Light* pLight, float3& lightPos)
{
    switch (pLight->getType())
    {
    case LightType::Directional:
        should_not_get_here();
        break;
    case LightType::Point:
        lightPos = ((PointLight*)pLight)->getWorldPosition();
        break;
    default:
        should_not_get_here();
    }
}

static void camClipSpaceToWorldSpace(const Camera* pCamera, float3& center, float& radius)
{
    // Store view frustum vertices in world space
    float3 viewFrustum[8];

    float3 clipSpace[8] =
    {
        float3(-1.0f, 1.0f, 0),
        float3(1.0f, 1.0f, 0),
        float3(1.0f, -1.0f, 0),
        float3(-1.0f, -1.0f, 0),
        float3(-1.0f, 1.0f, 1.0f),
        float3(1.0f, 1.0f, 1.0f),
        float3(1.0f, -1.0f, 1.0f),
        float3(-1.0f, -1.0f, 1.0f),
    };

    glm::mat4 invViewProj = pCamera->getInvViewProjMatrix();
    center = float3(0, 0, 0);

    // Average vertices of camera frustum
    for (uint32_t i = 0; i < 8; i++)
    {
        float4 crd = invViewProj * float4(clipSpace[i], 1);
        viewFrustum[i] = float3(crd) / crd.w;
        center += viewFrustum[i];
    }

    center *= (1.0f / 8.0f);

    // Calculate bounding sphere radius
    radius = 0;
    for (uint32_t i = 0; i < 8; i++)
    {
        float d = glm::length(center - viewFrustum[i]);
        radius = std::max(d, radius);
    }
}

void SimpleSM::ShadowPass::resetLightMat(const Camera *pCamera, const Light *pLight)
{
    float3 sceneCenter;
    float radius;

    camClipSpaceToWorldSpace(pCamera, sceneCenter, radius);
    createShadowMatrix(pLight, sceneCenter, radius, static_cast<float>(width) / height, lightVP);
    //mpVars["LightVP"].getParameterBlock()->setBlob(&lightVP, 0, sizeof(lightVP));
    mpVars["LightVP"]["lightVP"] = lightVP;

    getLightPosition(pLight, lightPos);
    mpVars["LightPos"]["lightPos"] = float4(lightPos, 1);
}

RenderPassReflection SimpleSM::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addOutput("output", "Shadow Map").bindFlags(ResourceBindFlags::UnorderedAccess | ResourceBindFlags::RenderTarget).format(ResourceFormat::RGBA32Float);
    reflector.addInput("worldPos", "World Position");
    reflector.addInput("worldNormal", "World Normal");
    //reflector.addInput("src");
    return reflector;
}

void SimpleSM::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    mShadowPass.resetDepthTexture();
    mShadowPass.resetLightMat(mpScene->getCamera().get(), mpScene->getLight(0).get());

    float4 clearColor(1, 0, 0, 1);
    pRenderContext->clearFbo(mShadowPass.pFbo.get(), clearColor, 1.0f, 0, FboAttachmentType::Depth | FboAttachmentType::Color);
  
    if (mpScene != nullptr)
        mpScene->render(pRenderContext, mShadowPass.mpGraphicsState.get(), mShadowPass.mpVars.get());

    mVisibilityPass.mpVars["shadowMap"] = mShadowPass.pDepth;
    mVisibilityPass.mpVars["shadowMapLinear"] = mShadowPass.pDepthLinear;
    mVisibilityPass.mpVars["worldPos"] = renderData["worldPos"]->asTexture();
    mVisibilityPass.mpVars["worldNorm"] = renderData["worldNormal"]->asTexture();
    mVisibilityPass.mpVars["LightVP"]["lightVP"] = mShadowPass.lightVP;
    mVisibilityPass.mpVars["LightPos"]["lightPos"] = float4(mShadowPass.lightPos, 1);
    mVisibilityPass.pFbo->attachColorTarget(renderData["output"]->asTexture(), 0);
    clearColor = float4(0, 0, 0, 1);
    pRenderContext->clearFbo(mVisibilityPass.pFbo.get(), clearColor, 1.0f, 0, FboAttachmentType::Color);
    mVisibilityPass.pPass->execute(pRenderContext, mVisibilityPass.pFbo);
}

void SimpleSM::renderUI(Gui::Widgets& widget)
{
    widget.slider<uint32_t>("Shadow Map Resolution - width", mShadowPass.width, 1, 2048*4);
    widget.slider<uint32_t>("Shadow Map Resolution - height", mShadowPass.height, 1, 2048*4);
}

void SimpleSM::setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene)
{
    mpScene = pScene;
    mShadowPass.mpProgram->addDefines(mpScene->getSceneDefines());
    mShadowPass.mpVars = GraphicsVars::create(mShadowPass.mpProgram->getReflector());
}

void SimpleSM::ShadowPass::resetDepthTexture()
{
    if (pDepth != nullptr && pDepth->getHeight() == height && pDepth->getWidth() == width)
        return;

    pDepth = Texture::create2D(width, height, ResourceFormat::D32Float, 1, 1, nullptr, Resource::BindFlags::DepthStencil | Resource::BindFlags::ShaderResource);
    pDepthLinear = Texture::create2D(width, height, ResourceFormat::R32Float, 1, 1, nullptr, Resource::BindFlags::RenderTarget | Resource::BindFlags::ShaderResource);
    pFbo->attachDepthStencilTarget(pDepth);
    pFbo->attachColorTarget(pDepthLinear, 0);
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
}
