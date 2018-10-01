#pragma once
#include "GLShader.h"

class FullscreenVS : public GLShader
{
public:
	FullscreenVS(const ShaderMetadata& Meta)
		: GLShader(Meta)
	{
	}
};

class FullscreenFS : public GLShader
{
public:
	FullscreenFS(const ShaderMetadata& Meta)
		: GLShader(Meta)
	{
	}
};

class SunFS : public GLShader
{
public:
	SunFS(const ShaderMetadata& Meta)
		: GLShader(Meta)
	{
	}
};