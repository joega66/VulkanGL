#pragma once
#include <Engine/Types.h>
#include <vulkan/vulkan.h>

class VulkanImageView;
class VulkanSampler;

class VulkanTextureID
{
public:
	VulkanTextureID(const VulkanTextureID&) = delete;
	VulkanTextureID& operator=(const VulkanTextureID&) = delete;

	VulkanTextureID() = default;
	VulkanTextureID(uint32 ID);
	VulkanTextureID(VulkanTextureID&& Other);
	~VulkanTextureID();

	VulkanTextureID& operator=(VulkanTextureID&& Other);

	inline const uint32& Get() const { return ID; }

protected:
	uint32 ID;
};

class VulkanBindlessResources
{
	friend class VulkanDevice; // EndFrame()
	friend class VulkanTextureID; // FreeResourceID()

public:
	VulkanBindlessResources(const VulkanBindlessResources&) = delete;
	VulkanBindlessResources& operator=(const VulkanBindlessResources&) = delete;

	VulkanBindlessResources(VkDevice Device, VkDescriptorType ResourceType, uint32 ResourceCount);
	~VulkanBindlessResources();

	VulkanTextureID CreateTextureID(const VulkanImageView& ImageView, const VulkanSampler& Sampler);

	inline VkDescriptorSetLayout GetLayout() const { return BindlessResourceSetLayout; }
	inline VkDescriptorSet GetSet() const { return BindlessResources; }

private:
	VkDevice Device;
	VkDescriptorSetLayout BindlessResourceSetLayout;
	VkDescriptorPool BindlessResourceDescriptorPool;
	VkDescriptorSet BindlessResources;
	uint32 CurrNumBindlessResources = 0;
	std::list<uint32> FreedThisFrame;
	std::list<uint32> Available;

	void FreeResourceID(uint32 ResourceID); // VulkanTextureID
	void EndFrame(); // VulkanDevice
};

extern VulkanBindlessResources* gBindlessTextures;