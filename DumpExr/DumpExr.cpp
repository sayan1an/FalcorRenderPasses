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
#include "DumpExr.h"

// Don't remove this. it's required for hot-reload to function properly
extern "C" __declspec(dllexport) const char* getProjDir()
{
    return PROJECT_DIR;
}

extern "C" __declspec(dllexport) void getPasses(Falcor::RenderPassLibrary& lib)
{
    lib.registerClass("DumpExr", "Dumps a texture to disk in exr format", DumpExr::create);
}

DumpExr::SharedPtr DumpExr::create(RenderContext* pRenderContext, const Dictionary& dict)
{
    SharedPtr pPass = SharedPtr(new DumpExr());

    for (const auto& v : dict) {
        if (v.key() == "featureIdx") {
            pPass->featureIdx = (uint32_t)v.val() == 0 ? 0 : 1;
            pPass->tagA = pPass->featureIdx == 0 ? "feature_" : "target_";
            pPass->tagB = pPass->featureIdx != 0 ? "feature_" : "target_";
        }
    }

    return pPass;
}

Dictionary DumpExr::getScriptingDictionary()
{
    Dictionary dict;
    dict["featureIdx"] = featureIdx;

    return dict;
}

RenderPassReflection DumpExr::reflect(const CompileData& compileData)
{
    // Define the required resources here
    RenderPassReflection reflector;
    reflector.addOutput("dstA", "OutputA");
    reflector.addOutput("dstB", "OutputB");
    reflector.addInput("srcA", "InputA");
    reflector.addInput("srcB", "InputB");
    return reflector;
}

void DumpExr::execute(RenderContext* pRenderContext, const RenderData& renderData)
{
    // renderData holds the requested resources
    const auto& pSrcTextureA = renderData["srcA"]->asTexture();
    const auto& pSrcTextureB = renderData["srcB"]->asTexture();
    const auto& pDstTextureA = renderData["dstA"]->asTexture();
    const auto& pDstTextureB = renderData["dstB"]->asTexture();
    
    pSrcTextureA->captureToFile(0, 0, "D:/results/" + tagA + std::to_string(index) + ".exr", Falcor::Bitmap::FileFormat::ExrFile, Falcor::Bitmap::ExportFlags::ExportAlpha | Falcor::Bitmap::ExportFlags::Uncompressed);
    pSrcTextureB->captureToFile(0, 0, "D:/results/" + tagB + std::to_string(index) + ".exr", Falcor::Bitmap::FileFormat::ExrFile, Falcor::Bitmap::ExportFlags::ExportAlpha | Falcor::Bitmap::ExportFlags::Uncompressed);

    pRenderContext->blit(pSrcTextureA->getSRV(), pDstTextureA->getRTV());
    pRenderContext->blit(pSrcTextureB->getSRV(), pDstTextureB->getRTV());

    index++;
}

void DumpExr::renderUI(Gui::Widgets& widget)
{
}
