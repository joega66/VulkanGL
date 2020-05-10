#include "MaterialShader.h"
#include "CameraProxy.h"
#include "SceneRenderer.h"
#include "Voxels.h"
#include <ECS/EntityManager.h>
#include <Engine/Engine.h>

template<EMeshType MeshType>
class GBufferPassVS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	GBufferPassVS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/GBufferVS.glsl", "main", EShaderStage::Vertex };
		return BaseInfo;
	}
};

template<EMeshType MeshType>
class GBufferPassFS : public MeshShader<MeshType>
{
	using Base = MeshShader<MeshType>;
public:
	GBufferPassFS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/GBufferFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

void CameraProxy::AddToGBufferPass(Engine& Engine, const MeshProxy& MeshProxy)
{
	constexpr EMeshType MeshType = EMeshType::StaticMesh;

	PipelineStateDesc PSODesc = {};
	PSODesc.RenderPass = GBufferRP;
	PSODesc.ShaderStages.Vertex = Engine.ShaderMap.FindShader<GBufferPassVS<MeshType>>();
	PSODesc.ShaderStages.Fragment = Engine.ShaderMap.FindShader<GBufferPassFS<MeshType>>();
	PSODesc.Viewport.Width = SceneDepth.GetWidth();
	PSODesc.Viewport.Height = SceneDepth.GetHeight();
	PSODesc.Layouts = { CameraDescriptorSet.GetLayout(), MeshProxy.GetSurfaceSet().GetLayout(), Engine.Device.GetTextures().GetLayout(), Engine.Device.GetSamplers().GetLayout() };

	const std::vector<VkDescriptorSet> DescriptorSets =
	{
		CameraDescriptorSet, MeshProxy.GetSurfaceSet(), Engine.Device.GetTextures().GetSet(), Engine.Device.GetSamplers().GetSet()
	};

	GBufferPass.push_back(MeshDrawCommand(Engine.Device, MeshProxy, PSODesc, DescriptorSets));
}

void SceneRenderer::RenderGBufferPass(CameraProxy& Camera, drm::CommandList& CmdList)
{
	CmdList.BeginRenderPass(Camera.GBufferRP);

	MeshDrawCommand::Draw(CmdList, Camera.GBufferPass);

	CmdList.EndRenderPass();
}