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
		psoDesc.pushConstantRanges.push_back(_Material->GetPushConstantRange());
		_Pipeline = device.CreatePipeline(psoDesc);
	}

	void Draw(gpu::CommandList& cmdList)
	{
		cmdList.BindPipeline(_Pipeline);

		cmdList.BindDescriptorSets(_Pipeline, static_cast<uint32>(_DescriptorSets.size()), _DescriptorSets.data());

		cmdList.PushConstants(_Pipeline, EShaderStage::Fragment, 0, sizeof(_Material->GetPushConstants()), &_Material->GetPushConstants());

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
	std::vector<VkDescriptorSet> _DescriptorSets;
	const std::vector<Submesh>& _Submeshes;
	const Material* _Material;
};