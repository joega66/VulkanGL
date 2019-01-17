#pragma once
#include <Platform/Platform.h>

enum EImageFormat
{
	IF_UNDEFINED,
	IF_R8_UINT,
	IF_R8_SINT,
	IF_R8G8_UINT,
	IF_R8G8_SINT,
	IF_R8_SRGB,
	IF_R8G8_SRGB,
	IF_R8G8B8_UINT,
	IF_R8G8B8_SINT,
	IF_R8G8B8_SRGB,
	IF_B8G8R8_UINT,
	IF_B8G8R8_SINT,
	IF_B8G8R8_SRGB,
	IF_R8G8B8A8_UINT,
	IF_R8G8B8A8_SINT,
	IF_R8G8B8A8_SRGB,
	IF_R8G8B8A8_UNORM,
	IF_B8G8R8A8_UINT,
	IF_B8G8R8A8_SINT,
	IF_B8G8R8A8_SRGB,
	IF_B8G8R8A8_UNORM,
	IF_R16_UINT,
	IF_R16_SINT,
	IF_R16_SFLOAT,
	IF_R16G16_UINT,
	IF_R16G16_SINT,
	IF_R16G16_SFLOAT,
	IF_R16G16B16_UINT,
	IF_R16G16B16_SINT,
	IF_R16G16B16_SFLOAT,
	IF_R16G16B16A16_UINT,
	IF_R16G16B16A16_SINT,
	IF_R16G16B16A16_SFLOAT,
	IF_R32_UINT,
	IF_R32_SINT,
	IF_R32_SFLOAT,
	IF_R32G32_UINT,
	IF_R32G32_SINT,
	IF_R32G32_SFLOAT,
	IF_R32G32B32_UINT,
	IF_R32G32B32_SINT,
	IF_R32G32B32_SFLOAT,
	IF_R32G32B32A32_UINT,
	IF_R32G32B32A32_SINT,
	IF_R32G32B32A32_SFLOAT,
	IF_D16_UNORM,
	IF_D32_SFLOAT,
	IF_S8_UINT,
	IF_D32_SFLOAT_S8_UINT,
	IF_D24_UNORM_S8_UINT,
	IF_BC2_UNORM_BLOCK,
};

enum class EResourceUsage
{
	None,
	RenderTargetable = 1 << 0,
	ShaderResource = 1 << 1,
	UnorderedAccess = 1 << 2,
	IndirectBuffer = 1 << 3,
	KeepCPUAccessible = 1 << 4,
	Cubemap = 1 << 5
};

enum class ELoadAction
{
	None,
	Clear,
	Load
};

enum class EStoreAction
{
	None,
	Store
};

enum class EDepthStencilAccess
{
	// Disable depth writes.
	None,
	// Enable depth writes.
	DepthWrite,
	// Disable depth writes and enable stencil attachment.
	StencilWrite,
	// Enable depth writes. (Really has same effect as DepthWrite.)
	DepthWriteStencilWrite,
	// Transitions depth to shader read for the depth aspect.
	DepthReadStencilWrite,
	// Transitions depth to shader read for the stencil aspect.
	DepthWriteStencilRead,
	// Transitions depth to shader read for depth/stencil aspect.
	DepthReadStencilRead,
};

class GLRenderResource
{
public:
	virtual void ReleaseGL() {}

	inline ~GLRenderResource()
	{
		ReleaseGL();
	}
};

CLASS(GLRenderResource);

class GLVertexBuffer : public GLRenderResource
{
public:
	EImageFormat Format;
	EResourceUsage Usage;

	GLVertexBuffer(EImageFormat Format, EResourceUsage Usage)
		: Format(Format), Usage(Usage)
	{
	}
};

CLASS(GLVertexBuffer);

class GLIndexBuffer : public GLRenderResource
{
public:
	EImageFormat Format;
	EResourceUsage Usage;
	uint32 IndexStride;

	GLIndexBuffer(uint32 IndexStride, EImageFormat Format, EResourceUsage Usage)
		: IndexStride(IndexStride), Format(Format), Usage(Usage)
	{
	}
};

CLASS(GLIndexBuffer);

class GLUniformBuffer : public GLRenderResource
{
public:
	bool bDirty = false;

	template<typename UniformType>
	void Set(const UniformType& UniformData)
	{
		check(GetSize() == sizeof(UniformType), "Size mismatch.");
		Data = std::make_shared<UniformType>(UniformData);
		bDirty = true;
	}

	const void* GetData() { return Data.get(); }
	virtual uint32 GetSize() = 0;

private:
	std::shared_ptr<void> Data;
};

CLASS(GLUniformBuffer);

class GLImage : public GLRenderResource
{
public:
	EImageFormat Format;
	uint32 Width;
	uint32 Height;
	EResourceUsage Usage;

	GLImage(EImageFormat Format, uint32 Width, uint32 Height, EResourceUsage UsageFlags)
		: Format(Format), Width(Width), Height(Height), Usage(UsageFlags)
	{
	}

	bool IsColor();
	bool IsDepthStencil();
	bool IsStencil();
	static bool IsDepth(EImageFormat Format);
	bool IsDepth();
};

CLASS(GLImage);

struct ClearDepthStencilValue
{
	float DepthClear;
	uint32 StencilClear;
};

class GLRenderTargetView : public GLRenderResource
{
public:
	GLImageRef Image;
	ELoadAction LoadAction;
	EStoreAction StoreAction;

	std::variant<std::array<float, 4>, ClearDepthStencilValue> ClearValue;

	GLRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const std::array<float, 4>& ClearValue);
	GLRenderTargetView(GLImageRef Image, ELoadAction LoadAction, EStoreAction StoreAction, const ClearDepthStencilValue& DepthStencil);
};

CLASS(GLRenderTargetView);

struct TextureCreateInfo
{
	int32 Width;
	int32 Height;
	uint8* Data;
};

struct CubemapCreateInfo
{
	// +X, -X, +Y, -Y, +Z, -Z
	std::array<TextureCreateInfo, 6> CubeFaces;
};