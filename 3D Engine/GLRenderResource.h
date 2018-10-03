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

enum EResourceUsageFlags
{
	RU_None,
	RU_RenderTargetable = 0x01,
	RU_ShaderResource = 0x02,
	RU_UnorderedAccess = 0x04,
	RU_IndirectBuffer = 0x08,
	RU_KeepCPUAccessible = 0x10,
};

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