#include "VulkanCache.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"

VulkanCache::VulkanCache(VulkanDevice& Device)
	: Device(Device)
{
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