#include "GL.h"

RenderCommandListRef GRenderCmdList;

void GLBeginRender()
{
	GRenderCmdList->BeginFrame();
}

void GLEndRender()
{
	GRenderCmdList->EndFrame();
}

void GLSetRenderTargets(uint32 NumRTs, const GLRenderTargetViewRef* ColorTargets, const GLRenderTargetViewRef DepthTarget, EDepthStencilAccess Access)
{
	GRenderCmdList->SetRenderTargets(NumRTs, ColorTargets, DepthTarget, Access);
}

void GLSetViewport(float X, float Y, float Width, float Height, float MinDepth, float MaxDepth)
{
	GRenderCmdList->SetViewport(X, Y, Width, Height, MinDepth, MaxDepth);
}

void GLSetDepthTest(bool bDepthTestEnable, EDepthCompareTest CompareTest)
{
	GRenderCmdList->SetDepthTest(bDepthTestEnable, CompareTest);
}

void GLSetStencilTest(bool bStencilTestEnable)
{
	GRenderCmdList->SetStencilTest(bStencilTestEnable);
}

void GLSetStencilState(ECompareOp CompareOp, EStencilOp FailOp, EStencilOp DepthFailOp, EStencilOp PassOp, uint32 CompareMask, uint32 WriteMask, uint32 Reference)
{
	GRenderCmdList->SetStencilState(CompareOp, FailOp, DepthFailOp, PassOp, CompareMask, WriteMask, Reference);
}

void GLSetRasterizerState(ECullMode CullMode, EFrontFace FrontFace, EPolygonMode PolygonMode, float LineWidth)
{
	GRenderCmdList->SetRasterizerState(CullMode, FrontFace, PolygonMode, LineWidth);
}

void GLSetColorMask(uint32 RenderTargetIndex, EColorChannel ColorWriteMask)
{
	GRenderCmdList->SetColorMask(RenderTargetIndex, ColorWriteMask);
}

void GLSetInputAssembly(EPrimitiveTopology Topology)
{
	GRenderCmdList->SetInputAssembly(Topology);
}

void GLSetGraphicsPipeline(GLShaderRef Vertex, GLShaderRef TessControl, GLShaderRef TessEval, GLShaderRef Geometry, GLShaderRef Fragment)
{
	GRenderCmdList->SetGraphicsPipeline(Vertex, TessControl, TessEval, Geometry, Fragment);
}

void GLSetUniformBuffer(GLShaderRef Shader, uint32 Location, GLUniformBufferRef UniformBuffer)
{
	GRenderCmdList->SetUniformBuffer(Shader, Location, UniformBuffer);
}

void GLSetUniformBuffer(GLShaderRef Shader, const std::string& Name, GLUniformBufferRef UniformBuffer)
{
	GRenderCmdList->SetUniformBuffer(Shader, Shader->GetUniformLocation(Name), UniformBuffer);
}

void GLSetShaderImage(GLShaderRef Shader, uint32 Location, GLImageRef Image, const SamplerState& Sampler)
{
	GRenderCmdList->SetShaderImage(Shader, Location, Image, Sampler);
}

void GLSetStorageBuffer(GLShaderRef Shader, uint32 Location, GLStorageBufferRef StorageBuffer)
{
	GRenderCmdList->SetStorageBuffer(Shader, Location, StorageBuffer);
}

void GLDrawIndexed(GLIndexBufferRef IndexBuffer, uint32 IndexCount, uint32 InstanceCount, uint32 FirstIndex, uint32 VertexOffset, uint32 FirstInstance)
{
	GRenderCmdList->DrawIndexed(IndexBuffer, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
}

void GLDraw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance)
{
	GRenderCmdList->Draw(VertexCount, InstanceCount, FirstVertex, FirstInstance);
}

GLIndexBufferRef GLCreateIndexBuffer(EImageFormat Format, uint32 NumIndices, EResourceUsage Usage, const void * Data)
{
	return GRenderCmdList->CreateIndexBuffer(Format, NumIndices, Usage, Data);
}

void GLSetVertexStream(uint32 Location, GLVertexBufferRef VertexBuffer)
{
	GRenderCmdList->SetVertexStream(Location, VertexBuffer);
}

GLVertexBufferRef GLCreateVertexBuffer(EImageFormat Format, uint32 NumElements, EResourceUsage Usage, const void * Data)
{
	return GRenderCmdList->CreateVertexBuffer(Format, NumElements, Usage, Data);
}

GLStorageBufferRef GLCreateStorageBuffer(uint32 Size, const void* Data, EResourceUsage Usage)
{
	return GRenderCmdList->CreateStorageBuffer(Size, Data, Usage);
}

GLImageRef GLCreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const uint8* Data)
{
	return GRenderCmdList->CreateImage(Width, Height, Format, UsageFlags, Data);
}

GLImageRef GLCreateCubemap(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsage UsageFlags, const CubemapCreateInfo& CubemapCreateInfo)
{
	return GRenderCmdList->CreateCubemap(Width, Height, Format, UsageFlags | EResourceUsage::Cubemap, CubemapCreateInfo);
}

GLRenderTargetViewRef GLCreateRenderTargetView(GLImageRef GLImage, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
{
	return GRenderCmdList->CreateRenderTargetView(GLImage, LoadAction, StoreAction, ClearValue);
}

GLRenderTargetViewRef GLCreateRenderTargetView(GLImageRef GLImage, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil)
{
	return GRenderCmdList->CreateRenderTargetView(GLImage, LoadAction, StoreAction, DepthStencil);
}

GLRenderTargetViewRef GLGetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue)
{
	return GRenderCmdList->GetSurfaceView(LoadAction, StoreAction, ClearValue);
}

void* GLLockBuffer(GLVertexBufferRef VertexBuffer, uint32 Size, uint32 Offset)
{
	return GRenderCmdList->LockBuffer(VertexBuffer, Size, Offset);
}

void GLUnlockBuffer(GLVertexBufferRef VertexBuffer)
{
	GRenderCmdList->UnlockBuffer(VertexBuffer);
}

void* GLLockBuffer(GLIndexBufferRef IndexBuffer, uint32 Size, uint32 Offset)
{
	return GRenderCmdList->LockBuffer(IndexBuffer, Size, Offset);
}

void GLUnlockBuffer(GLIndexBufferRef IndexBuffer)
{
	return GRenderCmdList->UnlockBuffer(IndexBuffer);
}

void GLRebuildResolutionDependents()
{
	GRenderCmdList->RebuildResolutionDependents();
}

std::string GLGetDeviceName()
{
	return GRenderCmdList->GetDeviceName();
}