#pragma once
#include <Engine/Types.h>
#include <vulkan/vulkan.h>

class VulkanImageView;
class VulkanSampler;

#define DECLARE_VULKAN_RESOURCE_ID(ResourceType) \
class ResourceType \
{ \
public: \
	ResourceType() = default; \
	ResourceType(uint32 ID); \
	void Release(); \
	inline const uint32& Get() const { return ID; } \
	inline operator uint32() const { return ID; } \
protected: \
	uint32 ID; \
}; \

DECLARE_VULKAN_RESOURCE_ID(VulkanTextureID);
DECLARE_VULKAN_RESOURCE_ID(VulkanSamplerID);

class VulkanBindlessResources
{
	friend class VulkanDevice;		// EndFrame
	friend class VulkanTextureID;	// FreeResourceID
	friend class VulkanSamplerID;	// FreeResourceID
	friend class VulkanSampler;		// CreateSamplerID

public:
	VulkanBindlessResources(const VulkanBindlessResources&) = delete;
	VulkanBindlessResources& operator=(const VulkanBindlessResources&) = delete;

	VulkanBindlessResources(VkDevice Device, VkDescriptorType ResourceType, uint32 ResourceCount);
	~VulkanBindlessResources();

	/** Create a texture ID for indexing into the texture2D array. */
	VulkanTextureID CreateTextureID(const VulkanImageView& ImageView);

	inline VkDescriptorSetLayout GetLayout() const { return BindlessResourceSetLayout; }
	inline VkDescriptorSet GetSet() const { return BindlessResources; }

private:
	VkDevice Device;
	VkDescriptorSetLayout BindlessResourceSetLayout;
	VkDescriptorPool BindlessResourceDescriptorPool;
	VkDescriptorSet BindlessResources;
	uint32 CurrNumBindlessResources = 0;
	std::list<uint32> Available;
	std::list<uint32> Released;

	/** Creates a SamplerID. Samplers call this automatically. */
	VulkanSamplerID CreateSamplerID(const VulkanSampler& Sampler);

	/** Allocates an ID into a bindless resource table. */
	uint32 AllocateResourceID();

	/** Release an ID. */
	void Release(uint32 ResourceID);

	/** Called in VulkanDevice::EndFrame(). */
	void EndFrame();
};

extern std::weak_ptr<VulkanBindlessResources> gBindlessTextures;
extern std::weak_ptr<VulkanBindlessResources> gBindlessSamplers;