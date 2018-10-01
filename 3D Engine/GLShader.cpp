#include "GLShader.h"

GLShaderCompilerRef GShaderCompiler;

void GLShaderCompiler::StoreShader(const std::string& Type, GLShaderRef Shader)
{
	Shaders[Type] = Shader;
}

GLShaderRef GLShaderCompiler::FindShader(const std::string& Type)
{
	if (Contains(Shaders, Type))
	{
		return Shaders[Type];
	}
	else
	{
		return nullptr;
	}
}