#include "GLShader.h"

GLShaderCompilerRef GShaderCompiler;

void GLShaderCompiler::StoreShader(std::type_index Type, GLShaderRef Shader)
{
	Shaders[Type] = Shader;
}

GLShaderRef GLShaderCompiler::FindShader(std::type_index Type)
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