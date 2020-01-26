#include "MaterialShader.h"
#include "SceneProxy.h"
#include "SceneRenderer.h"

template<EMeshType MeshType>
class LightingPassVS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	LightingPassVS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassVS.glsl", "main", EShaderStage::Vertex };
		return BaseInfo;
	}
};

template<EMeshType MeshType>
class LightingPassFS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	LightingPassFS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/LightingPassFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

void SceneProxy::AddToLightingPass(const MeshProxy& MeshProxy)
{
	static constexpr EMeshType MeshType = EMeshType::StaticMesh;

	ShaderStages ShaderStages =
	{
		ShaderMap.FindShader<LightingPassVS<MeshType>>(),
		nullptr,
		nullptr,
		nullptr,
		ShaderMap.FindShader<LightingPassFS<MeshType>>()
	};

	const EStaticDrawListType::EStaticDrawListType StaticDrawListType =
		MeshProxy.GetMaterial()->IsMasked() ?
		EStaticDrawListType::Masked : EStaticDrawListType::Opaque;

	LightingPass[StaticDrawListType].push_back(
		MeshDrawCommand(std::move(ShaderStages), MeshProxy)
	);
}

struct LightingPassDescriptorSets
{
	drm::DescriptorSetRef Scene;
	drm::DescriptorSetRef SceneTextures;

	void Set(drm::CommandList& CmdList, const MeshProxy& MeshProxy) const
	{
		const std::array<drm::DescriptorSetRef, 4> DescriptorSets =
		{
			Scene,
			MeshProxy.GetSurfaceSet(),
			MeshProxy.GetMaterialSet(),
			SceneTextures
		};

		CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());
	}
};

void SceneRenderer::RenderLightingPass(SceneProxy& Scene, drm::CommandList& CmdList)
{
	PipelineStateInitializer PSOInit = {};
	PSOInit.Viewport.Width = SceneDepth->Width;
	PSOInit.Viewport.Height = SceneDepth->Height;
	PSOInit.DepthStencilState.DepthCompareTest = EDepthCompareTest::Equal;
	PSOInit.DepthStencilState.DepthWriteEnable = false;

	LightingPassDescriptorSets DescriptorSets = { Scene.DescriptorSet, SceneTextures };

	MeshDrawCommand::Draw(Scene.LightingPass[EStaticDrawListType::Opaque], CmdList, DescriptorSets, PSOInit);
	MeshDrawCommand::Draw(Scene.LightingPass[EStaticDrawListType::Masked], CmdList, DescriptorSets, PSOInit);
}