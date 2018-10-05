#pragma once
#include "Platform.h"

// @todo-joe Move all GL Render Resources to this file? 

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
};

enum EResourceUsageFlags
{
	RU_None,
	RU_RenderTargetable = 0x01,
	RU_ShaderResource = 0x02,
	RU_UnorderedAccess = 0x04,
	RU_IndirectBuffer = 0x08,
	RU_KeepCPUAccessible = 0x10,
};

class GLVertexBuffer : public GLRenderResource
{
public:
	EImageFormat Format;
	EResourceUsageFlags Usage;

	GLVertexBuffer(EImageFormat Format, EResourceUsageFlags Usage)
		: Format(Format), Usage(Usage)
	{
	}
};

CLASS(GLVertexBuffer);

class GLIndexBuffer : public GLRenderResource
{
public:
	EImageFormat Format;
	EResourceUsageFlags Usage;
	uint32 IndexStride;

	GLIndexBuffer(uint32 IndexStride, EImageFormat Format, EResourceUsageFlags Usage)
		: IndexStride(IndexStride), Format(Format), Usage(Usage)
	{
	}
};

CLASS(GLIndexBuffer);

class GLUniformBuffer : public GLRenderResource
{
public:
	template<typename UniformType>
	void Set(const UniformType& UniformData)
	{
		check(Size() == sizeof(UniformType), "Size mismatch.");
		Data = std::make_shared<UniformType>(UniformData);
		MarkDirty();
	}

	const void* GetData() { return Data.get(); }
	virtual uint32 Size() = 0;

private:
	std::shared_ptr<void> Data;
	virtual void MarkDirty() = 0;
};

CLASS(GLUniformBuffer);