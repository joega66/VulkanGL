#include "MaterialShader.h"
#include "CameraProxy.h"
#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Engine/Engine.h>
#include <Components/Bounds.h>
#include <Renderer/MeshDrawCommand.h>

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

void SceneRenderer::RenderGBufferPass(const Camera& camera, CameraProxy& cameraProxy, gpu::CommandList& cmdList)
{
	cmdList.BeginRenderPass(cameraProxy._GBufferRP);

	const FrustumPlanes viewFrustumPlanes = camera.GetFrustumPlanes();

	std::vector<MeshDrawCommand> gBufferPass;

	for (auto entity : _ECS.GetEntities<MeshProxy>())
	{
		const auto& meshProxy = _ECS.GetComponent<MeshProxy>(entity);
		const auto& bounds = _ECS.GetComponent<Bounds>(entity);

		if (Physics::IsBoxInsideFrustum(viewFrustumPlanes, bounds.Box))
		{
			constexpr EMeshType meshType = EMeshType::StaticMesh;

			PipelineStateDesc psoDesc = {};
			psoDesc.renderPass = cameraProxy._GBufferRP;
			psoDesc.shaderStages.vertex = _ShaderLibrary.FindShader<GBufferPassVS<meshType>>();
			psoDesc.shaderStages.fragment = _ShaderLibrary.FindShader<GBufferPassFS<meshType>>();
			psoDesc.viewport.width = cameraProxy._SceneDepth.GetWidth();
			psoDesc.viewport.height = cameraProxy._SceneDepth.GetHeight();

			const std::vector<VkDescriptorSet> descriptorSets =
			{
				cameraProxy._CameraDescriptorSet, meshProxy.GetSurfaceSet(), _Device.GetTextures(), _Device.GetSamplers()
			};

			gBufferPass.push_back(MeshDrawCommand(_Device, meshProxy, psoDesc, descriptorSets));
		}
	}

	MeshDrawCommand::Draw(cmdList, gBufferPass);

	cmdList.EndRenderPass();
}