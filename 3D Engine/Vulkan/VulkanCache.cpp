#include "VulkanCache.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"

VulkanCache::VulkanCache(VulkanDevice& Device)
	: Device(Device)
{
}

gpu::Sampler VulkanCache::GetSampler(const SamplerDesc& SamplerDesc)
{
	const Crc Crc = Platform::CalculateCrc(&SamplerDesc, sizeof(SamplerDesc));

	if (auto SamplerIter = SamplerCache.find(Crc); SamplerIter != SamplerCache.end())
	{
		return SamplerIter->second;
	}
	else
	{
		SamplerCache.emplace(Crc, gpu::Sampler(Device, SamplerDesc));
		return SamplerCache.at(Crc);
	}
}

void VulkanCache::EndFrame()
{
	for (auto& Pipeline : PipelinesToDestroy)
	{
		vkDestroyPipeline(Device, Pipeline, nullptr);
	}

	PipelinesToDestroy.clear();
}

VulkanCache::~VulkanCache()
{
	for (const auto&[Crc, PipelineLayout] : PipelineLayoutCache)
	{
		vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
	}

	for (const auto&[Crc, RenderPass] : RenderPassCache)
	{
		vkDestroyRenderPass(Device, RenderPass, nullptr);
	}
}

void VulkanCache::RecompilePipelines()
{
	for (auto& [PSODesc, Pipeline] : GraphicsPipelineCache)
	{
		vkDestroyPipeline(Device, Pipeline->_Pipeline, nullptr);
		Pipeline->_Pipeline = CreatePipeline(PSODesc, Pipeline->_PipelineLayout);
	}

	for (auto& [Crc, Pipeline] : ComputePipelineCache)
	{
		vkDestroyPipeline(Device, Pipeline->_Pipeline, nullptr);
		Pipeline->_Pipeline = CreatePipeline(CrcToComputeDesc[Crc], Pipeline->_PipelineLayout);
	}
}