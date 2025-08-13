#include "NekiVK/Core/VulkanDescriptorPool.h"
#include "NekiVK/Debug/VKLogger.h"
#include <stdexcept>
#include <algorithm>


namespace Neki
{



VulkanDescriptorPool::VulkanDescriptorPool(const VKLogger& _logger, VKDebugAllocator& _deviceDebugAllocator, const VulkanDevice& _device, const std::uint32_t poolSizeCount, const VkDescriptorPoolSize* _poolSizes)
										  : logger(_logger), deviceDebugAllocator(_deviceDebugAllocator), device(_device)
{
	pool = VK_NULL_HANDLE;

	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::DESCRIPTOR_POOL, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::DESCRIPTOR_POOL, "Creating Descriptor Pool\n");

	//Create the pool
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;
	poolInfo.maxSets = 10;
	poolInfo.poolSizeCount = poolSizeCount;
	poolInfo.pPoolSizes = _poolSizes;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DESCRIPTOR_POOL, "Creating descriptor pool", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result{ vkCreateDescriptorPool(device.GetDevice(), &poolInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &pool) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DESCRIPTOR_POOL, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DESCRIPTOR_POOL, "(" + std::to_string(result) + ")\n");
		throw std::runtime_error("");
	}
}



VulkanDescriptorPool::~VulkanDescriptorPool()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::DESCRIPTOR_POOL,"Shutting down VulkanDescriptorPool\n");
	
	if (pool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(device.GetDevice(), pool, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
		pool = VK_NULL_HANDLE;
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::DESCRIPTOR_POOL,"  Descriptor Pool (and all allocated descriptor sets) Destroyed\n");
	}

	if (!ownedDescriptorSetLayouts.empty())
	{
		for (VkDescriptorSetLayout& l : ownedDescriptorSetLayouts)
		{
			if (l != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorSetLayout(device.GetDevice(), l, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
			}
		}
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::DESCRIPTOR_POOL,"  Descriptor Set Layouts Destroyed\n");
	}
	ownedDescriptorSetLayouts.clear();
}



VkDescriptorSet VulkanDescriptorPool::AllocateDescriptorSet(VkDescriptorSetLayout& _layout)
{
	// Track unique layouts only
	if (std::find(ownedDescriptorSetLayouts.begin(), ownedDescriptorSetLayouts.end(), _layout) == ownedDescriptorSetLayouts.end())
	{
		ownedDescriptorSetLayouts.push_back(_layout);
	}
	
	VkDescriptorSet descriptorSet;

	//Allocate the command buffer
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &_layout;
	
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DESCRIPTOR_POOL, "Allocating a single descriptor set", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result{ vkAllocateDescriptorSets(device.GetDevice(), &allocInfo, &descriptorSet) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DESCRIPTOR_POOL, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DESCRIPTOR_POOL," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}

	return descriptorSet;
}



std::vector<VkDescriptorSet> VulkanDescriptorPool::AllocateDescriptorSets(std::size_t _count, VkDescriptorSetLayout* _layouts)
{
	std::vector<VkDescriptorSet> descriptorSets(_count);

	for (std::size_t i{ 0 }; i<_count; ++i) {
		if (std::find(ownedDescriptorSetLayouts.begin(), ownedDescriptorSetLayouts.end(), _layouts[i]) == ownedDescriptorSetLayouts.end())
		{
			ownedDescriptorSetLayouts.push_back(_layouts[i]);
		}
	}

	//Allocate the command buffers
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(_count);
	allocInfo.pSetLayouts = _layouts;
	
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DESCRIPTOR_POOL, "Allocating " + std::to_string(_count) + " descriptor set" + std::string(_count == 1 ? "" : "s"), VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result{ vkAllocateDescriptorSets(device.GetDevice(), &allocInfo, descriptorSets.data()) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::COMMAND_POOL, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DESCRIPTOR_POOL," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}

	return descriptorSets;
}



void VulkanDescriptorPool::FreeDescriptorSet(VkDescriptorSet&_descriptorSet)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DESCRIPTOR_POOL, "Freeing 1 descriptor set");
	vkFreeDescriptorSets(device.GetDevice(), pool, 1, &_descriptorSet);
}



void VulkanDescriptorPool::FreeDescriptorSets(std::vector<VkDescriptorSet> &_descriptorSets)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DESCRIPTOR_POOL, "Freeing " + std::to_string(_descriptorSets.size()) + " descriptor set" + std::string(_descriptorSets.size() == 1 ? "" : "s"));
	vkFreeDescriptorSets(device.GetDevice(), pool, _descriptorSets.size(), _descriptorSets.data());
}



void VulkanDescriptorPool::Reset(VkDescriptorPoolResetFlags _flags)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::DESCRIPTOR_POOL, "Resetting command pool");
	vkResetDescriptorPool(device.GetDevice(), pool, _flags);
}



VkDescriptorPoolSize VulkanDescriptorPool::PresetSize(DESCRIPTOR_POOL_PRESET_SIZE _preset)
{
	VkDescriptorPoolSize size;
	switch (_preset)
	{
	case DESCRIPTOR_POOL_PRESET_SIZE::SINGLE_SSBO:
		size.descriptorCount = 1;
		size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		break;
	case DESCRIPTOR_POOL_PRESET_SIZE::SINGLE_UBO:
		size.descriptorCount = 1;
		size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		break;
	default:
		//Logger inaccessible in static function, sad.
		throw std::runtime_error(std::string("PresetSize() provided _preset (") + std::to_string(static_cast<std::underlying_type_t<DESCRIPTOR_POOL_PRESET_SIZE>>(_preset)) + std::string(") is not a valid DESCRIPTOR_POOL_PRESET_SIZE"));
	}

	return size;
}



}