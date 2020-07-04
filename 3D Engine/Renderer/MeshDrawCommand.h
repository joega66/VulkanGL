#pragma once
#include <GPU/GPU.h>
#include "MeshProxy.h"

class MeshDrawCommand
{
public:
	MeshDrawCommand(
		gpu::Device& device,
		const MeshProxy& meshProxy,
		PipelineStateDesc& psoDesc,
		const std::vector<VkDescriptorSet>& descriptorSets)
		: _Submeshes(meshProxy.GetSubmeshes())
		, _Material(meshProxy.GetMaterial())
		, _DescriptorSets(descriptorSets)
	{
		psoDesc.specInfo = meshProxy.GetSpecializationInfo();
		_Pipeline = device.CreatePipeline(psoDesc);
		_FragShader = psoDesc.shaderStages.fragment;
	}

	void Draw(gpu::CommandList& cmdList)
	{
		cmdList.BindPipeline(_Pipeline);

		cmdList.BindDescriptorSets(_Pipeline, _DescriptorSets.size(), _DescriptorSets.data());

		cmdList.PushConstants(_Pipeline, _FragShader, &_Material->GetPushConstants());

		for (const auto& submesh : _Submeshes)
		{
			cmdList.BindVertexBuffers(static_cast<uint32>(submesh.GetVertexBuffers().size()), submesh.GetVertexBuffers().data());

			cmdList.DrawIndexed(submesh.GetIndexBuffer(), submesh.GetIndexCount(), 1, 0, 0, 0, submesh.GetIndexType());
		}
	}

	static void Draw(gpu::CommandList& cmdList, std::vector<MeshDrawCommand>& meshDrawCommands)
	{
		for (auto& meshDrawCommand : meshDrawCommands)
		{
			meshDrawCommand.Draw(cmdList);
		}
	}

private:
	gpu::Pipeline _Pipeline;
	const gpu::Shader* _FragShader;
	std::vector<VkDescriptorSet> _DescriptorSets;
	const std::vector<Submesh>& _Submeshes;
	const Material* _Material;
};