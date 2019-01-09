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

void GLSetStencilTest(bool bStencilTestEnable)
{
	GRender->SetStencilTest(bStencilTestEnable);
}

void GLSetStencilState(ECompareOp CompareOp, EStencilOp FailOp, EStencilOp DepthFailOp, EStencilOp PassOp, uint32 CompareMask, uint32 WriteMask, uint32 Reference)
{
	GRender->SetStencilState(CompareOp, FailOp, DepthFailOp, PassOp, CompareMask, WriteMask, Reference);
}

void GLSetRasterizerState(ECullMode CullMode, EFrontFace FrontFace, EPolygonMode PolygonMode, float LineWidth)
{
	GRender->SetRasterizerState(CullMode, FrontFace, PolygonMode, LineWidth);
}

void GLSetColorMask(uint32 RenderTargetIndex, EColorChannel ColorWriteMask)
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

void GLSetUniformBuffer(GLShaderRef Shader, uint32 Location, GLUniformBufferRef UniformBuffer)
{
	GRender->SetUniformBuffer(Shader, Location, UniformBuffer);
}

void GLSetUniformBuffer(GLShaderRef Shader, const std::string& Name, GLUniformBufferRef UniformBuffer)
{
	GRender->SetUniformBuffer(Shader, Shader->GetUniformLocation(Name), UniformBuffer);
}

void GLSetShaderImage(GLShaderRef Shader, uint32 Location, GLImageRef Image, const SamplerState& Sampler)
{
	GRender->SetShaderImage(Shader, Location, Image, Sampler);
}

void GLDrawIndexed(GLIndexBufferRef IndexBuffer, uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, uint32 VertexOffset, uint32 FirstInstance)
{
	GRender->DrawIndexed(IndexBuffer, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
}

void GLDraw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance)
{
	GRender->Draw(VertexCount, InstanceCount, FirstVertex, FirstInstance);
}

GLIndexBufferRef GLCreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsage Usage, const void * Data)
{
	return GRender->CreateIndexBuffer(Format, NumIndices, Usage, Data);
}

void GLSetVertexStream(uint32 Location, GLVertexBufferRef VertexBuffer)
{
	GRender->SetVertexStream(Location, VertexBuffer);
}

GLVertexBufferRef GLCreateVertexBuffer(EImageFormat Format, uint32 NumElements, EResourceUsage Usage, const void * Data)
{
	return GRender->CreateVertexBuffer(Format, NumElements, Usage, Data);
}

GLImageRef GLCreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const uint8* Data)
{
	return GRender->CreateImage(Width, Height, Format, UsageFlags, Data);
}

GLImageRef GLCreateCubemap(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo)
{
	return GRender->CreateCubemap(Width, Height, Format, UsageFlags | EResourceUsage::Cubemap, CubemapCreateInfo);
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

void* GLLockBuffer(GLVertexBufferRef VertexBuffer, uint32 Size, uint32 Offset)
{
	return GRender->LockBuffer(VertexBuffer, Size, Offset);
}

void GLUnlockBuffer(GLVertexBufferRef VertexBuffer)
{
	GRender->UnlockBuffer(VertexBuffer);
}

void* GLLockBuffer(GLIndexBufferRef IndexBuffer, uint32 Size, uint32 Offset)
{
	return GRender->LockBuffer(IndexBuffer, Size, Offset);
}

void GLUnlockBuffer(GLIndexBufferRef IndexBuffer)
{
	return GRender->UnlockBuffer(IndexBuffer);
}

void GLRebuildResolutionDependents()
{
	GRender->RebuildResolutionDependents();
}

std::string GLGetDeviceName()
{
	return GRender->GetDeviceName();
}