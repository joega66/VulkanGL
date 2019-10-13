#pragma once
#include <DRMShader.h>

enum class EMeshType
{
	StaticMesh
};

template<EMeshType MeshType>
class MeshShader : public drm::Shader
{
public:
	MeshShader(const ShaderCompilationInfo& CompilationInfo)
		: drm::Shader(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		if constexpr (MeshType == EMeshType::StaticMesh)
		{
			Worker.SetDefine("STATIC_MESH");
		}
	}
};

template<EMeshType MeshType>
class MaterialShader : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	MaterialShader(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
		CompilationInfo.Bind("LocalToWorldUniform", LocalToWorld);
		CompilationInfo.Bind("Diffuse", Diffuse);
		CompilationInfo.Bind("Specular", Specular);
		CompilationInfo.Bind("Opacity", Opacity);
		CompilationInfo.Bind("Bump", Bump);
		CompilationInfo.Bind("HasOpacityMap", HasOpacityMap);
		CompilationInfo.Bind("HasSpecularMap", HasSpecularMap);
		CompilationInfo.Bind("HasBumpMap", HasBumpMap);
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
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

		Base::SetEnvironmentVariables(Worker);
	}

	ShaderBinding LocalToWorld;
	ShaderBinding Diffuse;
	ShaderBinding Specular;
	ShaderBinding Opacity;
	ShaderBinding Bump;
	SpecConstant HasSpecularMap;
	SpecConstant HasOpacityMap;
	SpecConstant HasBumpMap;
};