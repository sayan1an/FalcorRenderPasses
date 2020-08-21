from falcor import *

def render_graph_DefaultRenderGraph():
    g = RenderGraph("DefaultRenderGraph")
    loadRenderPassLibrary("ErrorMeasurePass.dll")
    loadRenderPassLibrary("BSDFViewer.dll")
    loadRenderPassLibrary("AccumulatePass.dll")
    loadRenderPassLibrary("Antialiasing.dll")
    loadRenderPassLibrary("BlitPass.dll")
    loadRenderPassLibrary("CSM.dll")
    loadRenderPassLibrary("DebugPasses.dll")
    loadRenderPassLibrary("DepthPass.dll")
    loadRenderPassLibrary("ExampleBlitPass.dll")
    loadRenderPassLibrary("ForwardLightingPass.dll")
    loadRenderPassLibrary("GBuffer.dll")
    loadRenderPassLibrary("ImageLoader.dll")
    loadRenderPassLibrary("SVGFPass.dll")
    loadRenderPassLibrary("MegakernelPathTracer.dll")
    loadRenderPassLibrary("MinimalPathTracer.dll")
    loadRenderPassLibrary("PixelInspectorPass.dll")
    loadRenderPassLibrary("PassLibraryTemplate.dll")
    loadRenderPassLibrary("SkyBox.dll")
    loadRenderPassLibrary("SSAO.dll")
    loadRenderPassLibrary("TemporalDelayPass.dll")
    loadRenderPassLibrary("ToneMapper.dll")
    loadRenderPassLibrary("Utils.dll")
    loadRenderPassLibrary("WhittedRayTracer.dll")
    loadRenderPassLibrary("WireframePass.dll")
    loadRenderPassLibrary("SimpleSM.dll")
    g.addPass(RenderPass("SimpleSM"), "simpleSM")
    g.addPass(RenderPass("GBufferRaster"), "gbRaster")
    g.addEdge("gbRaster.posW", "simpleSM.input")
    g.markOutput("simpleSM.output")
    return g

DefaultRenderGraph = render_graph_DefaultRenderGraph()
try: m.addGraph(DefaultRenderGraph)
except NameError: None