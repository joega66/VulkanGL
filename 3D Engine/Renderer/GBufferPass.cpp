#include "MaterialShader.h"
#include "CameraProxy.h"
#include "SceneRenderer.h"
#include <ECS/EntityManager.h>
#include <Engine/Engine.h>
#include <Renderer/Surface.h>

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

	for (auto entity : _ECS.GetEntities<SurfaceGroup>())
	{
		auto& surfaceGroup = _ECS.GetComponent<SurfaceGroup>(entity);

		const VkDescriptorSet descriptorSets[] = { cameraProxy._CameraDescriptorSet, surfaceGroup.GetSurfaceSet(), _Device.GetTextures(), _Device.GetSamplers() };

		surfaceGroup.Draw<true>(_Device, cmdList, std::size(descriptorSets), descriptorSets, [&] ()
		{
			constexpr EMeshType meshType = EMeshType::StaticMesh;

			PipelineStateDesc psoDesc = {};
			psoDesc.renderPass = cameraProxy._GBufferRP;
			psoDesc.shaderStages.vertex = _ShaderLibrary.FindShader<GBufferPassVS<meshType>>();
			psoDesc.shaderStages.fragment = _ShaderLibrary.FindShader<GBufferPassFS<meshType>>();
			psoDesc.viewport.width = cameraProxy._SceneDepth.GetWidth();
			psoDesc.viewport.height = cameraProxy._SceneDepth.GetHeight();

			return psoDesc;
		}, &viewFrustumPlanes);
	}

	cmdList.EndRenderPass();
}