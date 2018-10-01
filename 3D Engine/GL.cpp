#include "GL.h"

GLRef GRender;

void GLBeginRender()
{
	GRender->BeginRender();
}

void GLEndRender()
{
	GRender->EndRender();
}

void GLSetRenderTargets(uint32 NumRTs, const GLRenderTargetViewRef * ColorTargets, const GLRenderTargetViewRef DepthTarget, EDepthStencilAccess Access)
{
	GRender->SetRenderTargets(NumRTs, ColorTargets, DepthTarget, Access);
}

void GLSetViewport(float X, float Y, float Width, float Height, float MinDepth, float MaxDepth)
{
	GRender->SetViewport(X, Y, Width, Height, MinDepth, MaxDepth);
}

void GLSetDepthTest(bool bDepthTestEnable, EDepthCompareTest CompareTest)
{
	GRender->SetDepthTest(bDepthTestEnable, CompareTest);
}

void GLSetRasterizerState(ECullMode CullMode, EFrontFace FrontFace, EPolygonMode PolygonMode, float LineWidth)
{
	GRender->SetRasterizerState(CullMode, FrontFace, PolygonMode, LineWidth);
}

void GLSetColorMask(uint32 RenderTargetIndex, EColorWriteMask ColorWriteMask)
{
	GRender->SetColorMask(RenderTargetIndex, ColorWriteMask);
}

void GLSetInputAssembly(EPrimitiveTopology Topology)
{
	GRender->SetInputAssembly(Topology);
}

void GLSetGraphicsPipeline(GLShaderRef Vertex, GLShaderRef TessControl, GLShaderRef TessEval, GLShaderRef Geometry, GLShaderRef Fragment)
{
	GRender->SetGraphicsPipeline(Vertex, TessControl, TessEval, Geometry, Fragment);
}

void GLSetShaderResource(GLShaderRef Shader, uint32 Location, GLImageRef Image, const SamplerState & Sampler)
{
	GRender->SetShaderResource(Shader, Location, Image, Sampler);
}

void GLDraw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance)
{
	GRender->Draw(VertexCount, InstanceCount, FirstVertex, FirstInstance);
}

GLImageRef GLCreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceCreateFlags CreateFlags)
{
	return GRender->CreateImage(Width, Height, Format, CreateFlags);
}

void GLResizeImage(GLImageRef Image, uint32 Width, uint32 Height)
{
	GRender->ResizeImage(Image, Width, Height);
}

GLRenderTargetViewRef GLCreateRenderTargetView(GLImageRef GLImage, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
{
	return GRender->CreateRenderTargetView(GLImage, LoadAction, StoreAction, ClearValue);
}

GLRenderTargetViewRef GLCreateRenderTargetView(GLImageRef GLImage, ELoadAction LoadAction, EStoreAction StoreAction, float DepthClear, uint32 StencilClear)
{
	return GRender->CreateRenderTargetView(GLImage, LoadAction, StoreAction, DepthClear, StencilClear);
}

GLRenderTargetViewRef GLGetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
{
	return GRender->GetSurfaceView(LoadAction, StoreAction, ClearValue);
}

void GLRebuildResolutionDependents()
{
	GRender->RebuildResolutionDependents();
}

std::string GLGetDeviceName()
{
	return GRender->GetDeviceName();
}
