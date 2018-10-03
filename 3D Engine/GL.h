#pragma once
#include "Platform.h"
#include "GLImage.h"
#include "GLShader.h"

enum EDepthCompareTest
{
	Depth_Never,
	Depth_Less,
	Depth_Equal,
	Depth_LEqual,
	Depth_Greater,
	Depth_NEqual,
	Depth_GEqual,
	Depth_Always
};

enum EPolygonMode
{
	PM_Fill,
	PM_Line,
	PM_Point
};

enum EFrontFace
{
	FF_CCW,
	FF_CW
};

enum ECullMode
{
	CM_None,
	CM_Front,
	CM_Back,
	CM_FrontAndBack
};

enum EColorWriteMask
{
	Color_R = 0x01,
	Color_G = 0x02,
	Color_B = 0x04,
	Color_A = 0x08,
	Color_RGBA = Color_R | Color_G | Color_B | Color_A
};

enum EPrimitiveTopology
{
	PT_PointList,
	PT_LineList,
	PT_LineStrip,
	PT_TriangleList,
	PT_TriangleStrip,
	PT_TriangleFan
};

enum ESamplerAddressMode
{
	SAM_Repeat,
	SAM_MirroredRepeat,
	SAM_ClampToEdge,
	SAM_ClampToBorder,
	SAM_MirrorClampToEdge
};

enum ESamplerMipmapMode
{
	SMM_Nearest,
	SMM_Linear
};

enum EFilter
{
	Filter_Nearest,
	Filter_Linear,
	Filter_Cubic
};

struct SamplerState
{
	EFilter Filter;
	ESamplerAddressMode SAM;
	ESamplerMipmapMode SMM;

	SamplerState(EFilter Filter = Filter_Linear, ESamplerAddressMode SAM = SAM_ClampToBorder, ESamplerMipmapMode SMM = SMM_Linear)
		: Filter(Filter), SAM(SAM), SMM(SMM)
	{
	}
};

/** Graphics Library Interface */
class GL : public GLRenderResource
{
public:
	virtual void InitGL() = 0;
	virtual void ReleaseGL() = 0;

	virtual void BeginRender() = 0;
	virtual void EndRender() = 0;

	virtual void SetRenderTargets(uint32 NumRTs, const GLRenderTargetViewRef* ColorTargets, const GLRenderTargetViewRef DepthTarget, EDepthStencilAccess Access) = 0;
	virtual void SetViewport(float X, float Y, float Width, float Height, float MinDepth = 0.0f, float MaxDepth = 1.0f) = 0;
	virtual void SetDepthTest(bool bDepthTestEnable, EDepthCompareTest CompareTest = Depth_Less) = 0;
	virtual void SetRasterizerState(ECullMode CullMode, EFrontFace FrontFace = FF_CCW, EPolygonMode PolygonMode = PM_Fill, float LineWidth = 1.0f) = 0;
	virtual void SetColorMask(uint32 RenderTargetIndex, EColorWriteMask ColorWriteMask) = 0;
	virtual void SetInputAssembly(EPrimitiveTopology Topology) = 0;
	virtual void SetGraphicsPipeline(
		GLShaderRef Vertex,
		GLShaderRef TessControl,
		GLShaderRef TessEval,
		GLShaderRef Geometry,
		GLShaderRef Fragment
	) = 0;
	virtual void SetUniformBuffer(GLShaderRef Shader, uint32 Location, GLUniformBufferRef UniformBuffer) = 0;
	virtual void SetShaderImage(GLShaderRef Shader, uint32 Location, GLImageRef Image, const SamplerState& Sampler) = 0;
	virtual void Draw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance) = 0;
	virtual GLUniformBufferRef CreateUniformBuffer(uint32 Size, const void* Data) = 0;
	virtual GLImageRef CreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsageFlags UsageFlags) = 0;
	virtual void ResizeImage(GLImageRef Image, uint32 Width, uint32 Height) = 0;
	virtual GLRenderTargetViewRef CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue) = 0;
	virtual GLRenderTargetViewRef CreateRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, float DepthClear, uint32 StencilClear) = 0;
	virtual GLRenderTargetViewRef GetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue) = 0;
	virtual void RebuildResolutionDependents() = 0;
	virtual std::string GetDeviceName() = 0;
};

CLASS(GL);

extern GLRef GRender;

void GLBeginRender();
void GLEndRender();
void GLSetRenderTargets(uint32 NumRTs, const GLRenderTargetViewRef* ColorTargets, const GLRenderTargetViewRef DepthTarget, EDepthStencilAccess Access);
void GLSetViewport(float X, float Y, float Width, float Height, float MinDepth = 0.0f, float MaxDepth = 1.0f);
void GLSetDepthTest(bool bDepthTestEnable, EDepthCompareTest CompareTest = Depth_Less);
void GLSetRasterizerState(ECullMode CullMode, EFrontFace FrontFace = FF_CCW, EPolygonMode PolygonMode = PM_Fill, float LineWidth = 1.0f);
void GLSetColorMask(uint32 RenderTargetIndex, EColorWriteMask ColorWriteMask);
void GLSetInputAssembly(EPrimitiveTopology Topology);
void GLSetGraphicsPipeline(GLShaderRef Vertex, GLShaderRef TessControl, GLShaderRef TessEval, GLShaderRef Geometry, GLShaderRef Fragment);
void GLSetUniformBuffer(GLShaderRef Shader, uint32 Location, GLUniformBufferRef UniformBuffer);
void GLSetShaderImage(GLShaderRef Shader, uint32 Location, GLImageRef Image, const SamplerState& Sampler);
void GLDraw(uint32 VertexCount, uint32 InstanceCount, uint32 FirstVertex, uint32 FirstInstance);
GLImageRef GLCreateImage(uint32 Width, uint32 Height, EImageFormat Format, EResourceUsageFlags UsageFlags);
void GLResizeImage(GLImageRef Image, uint32 Width, uint32 Height);
GLRenderTargetViewRef GLCreateRenderTargetView(GLImageRef GLImage, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue);
GLRenderTargetViewRef GLCreateRenderTargetView(GLImageRef GLImage, ELoadAction LoadAction, EStoreAction StoreAction, float DepthClear, uint32 StencilClear);
GLRenderTargetViewRef GLGetSurfaceView(ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue);
void GLRebuildResolutionDependents();
std::string GLGetDeviceName();

// @todo-joe Pass by const ref...
template<typename UniformBufferType>
GLUniformBufferRef GLCreateUniformBuffer(const UniformBufferType& Data)
{
	return GRender->CreateUniformBuffer(sizeof(UniformBufferType), &Data);
}

template<typename ShaderType> // std::enable_if<std::is_base_of<GLShader, ShaderType>>, or something...
static GLShaderRef GLCreateShader()
{
	const std::string& Type = typeid(ShaderType).name();
	GLShaderRef Shader = GShaderCompiler->FindShader(Type);
	check(Shader, "The shader compiler is missing shader with type %s!", Type.c_str());
	return Shader;
}