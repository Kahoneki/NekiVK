#ifndef DESCRIPTORPOOL_H
#define DESCRIPTORPOOL_H


#include "VulkanDevice.h"


//Responsible for:
//-The initialisation, ownership, and clean shutdown of a VkDescriptorPool object
//-The allocation of VkDescriptorSet objects from the VkDescriptorPool member
//
//The underlying descriptor pool is not accessible, accessors must use provided functionality
namespace Neki
{

enum class DESCRIPTOR_POOL_PRESET_SIZE
{
	SINGLE_SSBO,
	SINGLE_UBO,
	SINGLE_COMBINED_IMAGE_SAMPLER,
};

class VulkanDescriptorPool
{
public:
	explicit VulkanDescriptorPool(const VKLogger& _logger,
							   VKDebugAllocator& _deviceDebugAllocator,
							   const VulkanDevice& _device,
							   const std::uint32_t _poolSizeCount,
							   const VkDescriptorPoolSize* _poolSizes);

	~VulkanDescriptorPool();

	//Allocate a single descriptor set from this pool - passing _layout to this function passes ownership to the VulkanDescriptorPool object
	[[nodiscard]] VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout& _layout);

	//Allocate multiple descriptor sets from this pool - passing _layouts to this function passes ownership to the VulkanDescriptorPool object
	[[nodiscard]] std::vector<VkDescriptorSet> AllocateDescriptorSets(std::size_t _count, VkDescriptorSetLayout* _layouts);

	//Free a specific descriptor set
	void FreeDescriptorSet(VkDescriptorSet& _descriptorSet);

	//Free a vector of descriptor sets
	void FreeDescriptorSets(std::vector<VkDescriptorSet>& _descriptorSets);

	//Reset the entire pool, invalidating all allocated descriptor sets
	void Reset(VkDescriptorPoolResetFlags _flags = 0);


	//Use Neki::VulkanDescriptorPool::PresetSize() to get a default size that can be passed to VulkanDescriptorPool's constructor
	static VkDescriptorPoolSize PresetSize(DESCRIPTOR_POOL_PRESET_SIZE _preset);


private:
	//Dependency injections from VKApp
	const VKLogger& logger;
	VKDebugAllocator& deviceDebugAllocator;
	const VulkanDevice& device;
		
	VkDescriptorPool pool;
	std::vector<VkDescriptorSetLayout> ownedDescriptorSetLayouts;
};
}



#endif