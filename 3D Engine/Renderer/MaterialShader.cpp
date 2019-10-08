#include "MaterialShader.h"
#include "SceneProxy.h"

void MaterialShader::SetEnvironmentVariables(ShaderCompilerWorker& Worker)
{
	constexpr uint32 MATERIAL_SET = 1;
	constexpr uint32 LOCAL_TO_WORLD_BINDING = 0;
	constexpr uint32 DIFFUSE_BINDING = 1;
	constexpr uint32 SPECULAR_BINDING = 2;
	constexpr uint32 OPACITY_BINDING = 3;
	constexpr uint32 BUMP_BINDING = 4;

	Worker.SetDefine("MATERIAL_SET", MATERIAL_SET);
	Worker.SetDefine("LOCAL_TO_WORLD_BINDING", LOCAL_TO_WORLD_BINDING);
	Worker.SetDefine("DIFFUSE_BINDING", DIFFUSE_BINDING);
	Worker.SetDefine("SPECULAR_BINDING", SPECULAR_BINDING);
	Worker.SetDefine("OPACITY_BINDING", OPACITY_BINDING);
	Worker.SetDefine("BUMP_BINDING", BUMP_BINDING);

	SceneProxy::SetEnvironmentVariables(Worker);
}