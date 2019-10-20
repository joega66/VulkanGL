#include "Voxels.h"
#include "SceneRenderer.h"
#include "MaterialShader.h"
#include "SceneProxy.h"
#include <Components/CMaterial.h>

const glm::uvec2 gVoxelGridSize = { 128, 128 };

template<EMeshType MeshType>
class VoxelsVS : public MaterialShader<MeshType>
{
	using Base = MaterialShader<MeshType>;
public:
	VoxelsVS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/VoxelsVS.glsl", "main", EShaderStage::Vertex };
		return BaseInfo;
	}
};

template<EMeshType MeshType>
class VoxelsGS : public MaterialShader<MeshType>
{
	using Base = MaterialShader<MeshType>;
public:
	VoxelsGS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/VoxelsGS.glsl", "main", EShaderStage::Geometry };
		return BaseInfo;
	}
};

template<EMeshType MeshType>
class VoxelsFS : public MaterialShader<MeshType>
{
	using Base = MaterialShader<MeshType>;
public:
	VoxelsFS(const ShaderCompilationInfo& CompilationInfo)
		: Base(CompilationInfo)
	{
	}

	static void SetEnvironmentVariables(ShaderCompilerWorker& Worker)
	{
		Base::SetEnvironmentVariables(Worker);
	}

	static const ShaderInfo& GetShaderInfo()
	{
		static ShaderInfo BaseInfo = { "../Shaders/VoxelsFS.glsl", "main", EShaderStage::Fragment };
		return BaseInfo;
	}
};

VoxelizationPass::VoxelizationPass(const MeshElement& Element, CMaterial& Material, drm::UniformBufferRef LocalToWorldUniform)
	: Element(Element)
{
	static constexpr EMeshType MeshType = EMeshType::StaticMesh;

	auto VertShader = *ShaderMapRef<VoxelsVS<MeshType>>();
	auto GeomShader = *ShaderMapRef<VoxelsGS<MeshType>>();
	auto FragShader = *ShaderMapRef<VoxelsFS<MeshType>>();

	SpecInfo.Add(FragShader->HasSpecularMap, Material.HasSpecularMap());
	SpecInfo.Add(FragShader->HasOpacityMap, Material.IsMasked());

	const SamplerState LinearSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };
	const SamplerState BumpSampler = { EFilter::Linear, ESamplerAddressMode::Repeat, ESamplerMipmapMode::Linear };

	DescriptorSet = drm::CreateDescriptorSet();

	DescriptorSet->Write(LocalToWorldUniform, GeomShader->LocalToWorld);
	DescriptorSet->Write(Material.Diffuse, LinearSampler, FragShader->Diffuse);
	DescriptorSet->Write(Material.Specular, LinearSampler, FragShader->Specular);
	DescriptorSet->Write(Material.Opacity, LinearSampler, FragShader->Opacity);
	DescriptorSet->Write(Material.Bump, BumpSampler, FragShader->Bump);

	DescriptorSet->Update();

	this->VertShader = VertShader;
	this->GeomShader = GeomShader;
	this->FragShader = FragShader;
}

void VoxelizationPass::BindDescriptorSets(RenderCommandList& CmdList, const SceneProxy& Scene) const
{
	const std::array<drm::DescriptorSetRef, 3> DescriptorSets = 
	{
		Scene.DescriptorSet,
		DescriptorSet,
		Scene.VoxelsDescriptorSet
	};

	CmdList.BindDescriptorSets(DescriptorSets.size(), DescriptorSets.data());
}

void VoxelizationPass::SetPipelineState(PipelineStateInitializer& PSOInit) const
{
	PSOInit.SpecializationInfos[(int32)EShaderStage::Fragment] = SpecInfo;

	PSOInit.GraphicsPipelineState =
	{
		VertShader,
		nullptr,
		nullptr,
		GeomShader,
		FragShader
	};
}

void VoxelizationPass::Draw(RenderCommandList& CmdList) const
{
	CmdList.BindVertexBuffers(Element.VertexBuffers.size(), Element.VertexBuffers.data());
	CmdList.DrawIndexed(Element.IndexBuffer, Element.IndexCount, 1, 0, 0, 0);
}

void SceneRenderer::RenderVoxels(SceneProxy& Scene, RenderCommandList& CmdList)
{
	RenderPassInitializer RPInit = { 0 }; // Disable ROP
	RPInit.RenderArea.Extent = gVoxelGridSize;

	CmdList.BeginRenderPass(RPInit);

	PipelineStateInitializer PSOInit;
	PSOInit.DepthStencilState.DepthTestEnable = false;
	PSOInit.DepthStencilState.DepthCompareTest = EDepthCompareTest::Always;
	PSOInit.Viewport.Width = gVoxelGridSize.x;
	PSOInit.Viewport.Height = gVoxelGridSize.y;

	Scene.VoxelsPass.Draw(CmdList, PSOInit, Scene);

	CmdList.EndRenderPass();
}