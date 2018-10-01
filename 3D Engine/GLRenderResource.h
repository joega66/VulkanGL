#pragma once
#include "Platform.h"

class GLRenderResource
{
public:
	virtual void ReleaseGL() {};

	inline ~GLRenderResource()
	{
		ReleaseGL();
	}
};

CLASS(GLRenderResource);

enum EResourceCreateFlags
{
	RF_RenderTargetable = 0x01,
	RF_ShaderResource = 0x02,
	RF_UnorderedAccess = 0x04,
};