#include "MaterialShader.h"
#include "SceneProxy.h"
#include "SceneRenderer.h"

template<EMeshType MeshType>
class DepthPrepassVS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	DepthPrepassVS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/DepthPrepass.glsl", "main", EShaderStage::Vertex };
		return BaseInfo;
	}
};

template<EMeshType MeshType>
class DepthPrepassFS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	DepthPrepassFS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/DepthPrepass.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

void SceneProxy::AddToDepthPrepass(DRMShaderMap& ShaderMap, const MeshProxy& MeshProxy)
{
	constexpr EMeshType MeshType = EMeshType::StaticMesh;

	ShaderStages ShaderStages =
	{
		ShaderMap.FindShader<DepthPrepassVS<MeshType>>(),
		nullptr,
		nullptr,
		nullptr,
		MeshProxy.GetMaterial()->IsMasked() ? ShaderMap.FindShader<DepthPrepassFS<MeshType>>() : nullptr
	};

	DepthPrepass.push_back(MeshDrawCommand(std::move(ShaderStages), MeshProxy));
}

struct DepthPrepassDescriptorSets
{
	drm::DescriptorSetRef SceneDescriptorSet;

	void Set(drm::CommandList& CmdList, const MeshProxy& MeshProxy) const
	{
		std::array<drm::DescriptorSetRef, 3> DescriptorSets =
		{
			SceneDescriptorSet,
			MeshProxy.GetSurfaceSet(),
			MeshProxy.GetMaterialSet()
		};

		CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());
	}
};

void SceneRenderer::RenderDepthPrepass(SceneProxy& Scene, drm::CommandList& CmdList)
{
	CmdList.BeginRenderPass(DepthRP);

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = &DepthRP;
	PSODesc.Viewport.Width = SceneDepth.GetWidth();
	PSODesc.Viewport.Height = SceneDepth.GetHeight();

	DepthPrepassDescriptorSets DescriptorSets = { Scene.CameraDescriptorSet };

	MeshDrawCommand::Draw(Scene.DepthPrepass, CmdList, DescriptorSets, PSODesc);

	CmdList.EndRenderPass();
}