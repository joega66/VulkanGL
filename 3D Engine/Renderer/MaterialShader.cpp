#include "MaterialShader.h"
#include "SceneProxy.h"

void MaterialShader::SetEnvironmentVariables(ShaderCompilerWorker& Worker)
{
	constexpr uint32 MATERIAL_SET = 1;
	constexpr uint32 LOCAL_TO_WORLD_BINDING = 0;
	constexpr uint32 DIFFUSE_BINDING = 1;

	Worker.SetDefine("MATERIAL_SET", MATERIAL_SET);
	Worker.SetDefine("LOCAL_TO_WORLD_BINDING", LOCAL_TO_WORLD_BINDING);
	Worker.SetDefine("DIFFUSE_BINDING", DIFFUSE_BINDING);

	SceneProxy::SetEnvironmentVariables(Worker);
}