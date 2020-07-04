#include "MaterialShader.h"
#include "CameraProxy.h"
#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Engine/Engine.h>

template<EMeshType meshType>
class GBufferPassVS : public MeshShader<meshType>
{
	using Base = MeshShader<meshType>;
public:
	GBufferPassVS(const ShaderCompilationInfo& compilationInfo)
		: Base(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		Base::SetEnvironmentVariables(worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo info = { "../Shaders/GBufferVS.glsl", "main", EShaderStage::Vertex };
		return info;
	}
};

template<EMeshType meshType>
class GBufferPassFS : public MeshShader<meshType>
{
	using Base = MeshShader<meshType>;
public:
	GBufferPassFS(const ShaderCompilationInfo& compilationInfo)
		: Base(compilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& worker)
	{
		Base::SetEnvironmentVariables(worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo baseInfo = { "../Shaders/GBufferFS.glsl", "main", EShaderStage::Fragment };
		return baseInfo;
	}
};

void CameraProxy::AddToGBufferPass(Engine& engine, const MeshProxy& meshProxy)
{
	constexpr EMeshType meshType = EMeshType::StaticMesh;

	PipelineStateDesc psoDesc = {};
	psoDesc.renderPass = _GBufferRP;
	psoDesc.shaderStages.vertex = engine.ShaderLibrary.FindShader<GBufferPassVS<meshType>>();
	psoDesc.shaderStages.fragment = engine.ShaderLibrary.FindShader<GBufferPassFS<meshType>>();
	psoDesc.viewport.width = _SceneDepth.GetWidth();
	psoDesc.viewport.height = _SceneDepth.GetHeight();

	const std::vector<VkDescriptorSet> descriptorSets =
	{
		_CameraDescriptorSet, meshProxy.GetSurfaceSet(), engine.Device.GetTextures().GetSet(), engine.Device.GetSamplers().GetSet()
	};

	_GBufferPass.push_back(MeshDrawCommand(engine.Device, meshProxy, psoDesc, descriptorSets));
}

void SceneRenderer::RenderGBufferPass(CameraProxy& camera, gpu::CommandList& cmdList)
{
	cmdList.BeginRenderPass(camera._GBufferRP);

	MeshDrawCommand::Draw(cmdList, camera._GBufferPass);

	cmdList.EndRenderPass();
}