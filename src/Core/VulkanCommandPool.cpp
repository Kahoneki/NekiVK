#include "NekiVK/Core/VulkanCommandPool.h"
#include "NekiVK/Debug/VKLogger.h"
#include <stdexcept>

namespace Neki
{



VulkanCommandPool::VulkanCommandPool(const VKLogger& _logger, VKDebugAllocator& _deviceDebugAllocator, const VulkanDevice& _device, VK_COMMAND_POOL_TYPE _poolType)
									: logger(_logger), deviceDebugAllocator(_deviceDebugAllocator), device(_device)
{
	pool = VK_NULL_HANDLE;
	poolType = _poolType;

	//Verify that the pool type is an available type
	if (poolType >= VK_COMMAND_POOL_TYPE::MAX_ENUM)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::COMMAND_POOL, "Provided _poolType (" + std::to_string(static_cast<std::underlying_type_t<VK_COMMAND_POOL_TYPE>>(_poolType)) + ") is an invalid command pool type.\n");
		throw std::runtime_error("");
	}
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::COMMAND_POOL, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::COMMAND_POOL, "Creating Command Pool\n");

	//Create the pool
	std::size_t queueFamilyIndex{};
	if (poolType == VK_COMMAND_POOL_TYPE::GRAPHICS)
	{
		queueFamilyIndex = device.GetGraphicsQueueFamilyIndex();
	}
	else if (poolType == VK_COMMAND_POOL_TYPE::COMPUTE)
	{
		//Todo: add compute queue support
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::COMMAND_POOL, "Provided _poolType (COMPUTE) is not currently supported.\n");
		throw std::runtime_error("");
	}
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;
	poolInfo.queueFamilyIndex = queueFamilyIndex;
	poolInfo.flags = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::COMMAND_POOL, "Creating command pool for queue family " + std::to_string(queueFamilyIndex) + " (" + PoolTypeToString(poolType) + ")", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result{ vkCreateCommandPool(device.GetDevice(), &poolInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &pool) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::DEVICE, result == VK_SUCCESS ? "success" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::COMMAND_POOL," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}
}



VulkanCommandPool::~VulkanCommandPool()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::COMMAND_POOL,"Shutting down VulkanCommandPool\n");
	
	if (pool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(device.GetDevice(), pool, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
		pool = VK_NULL_HANDLE;
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::COMMAND_POOL,"  Command Pool (and all allocated command buffers) Destroyed\n");
	}
}



VkCommandBuffer VulkanCommandPool::AllocateCommandBuffer(VkCommandBufferLevel _level)
{
	VkCommandBuffer buffer;

	//Allocate the command buffer
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.commandPool = pool;
	allocInfo.level = _level;
	allocInfo.commandBufferCount = 1;
	
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::COMMAND_POOL, "Allocating a single " + std::string(_level == VK_COMMAND_BUFFER_LEVEL_PRIMARY ? "primary" : "secondary") + " command buffer for " + PoolTypeToString(poolType) + " pool", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result{ vkAllocateCommandBuffers(device.GetDevice(), &allocInfo, &buffer) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::COMMAND_POOL, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::COMMAND_POOL," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}

	return buffer;
}



std::vector<VkCommandBuffer> VulkanCommandPool::AllocateCommandBuffers(std::uint32_t _count, VkCommandBufferLevel _level)
{
	std::vector<VkCommandBuffer> buffers(_count);

	//Allocate the command buffers
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.commandPool = pool;
	allocInfo.level = _level;
	allocInfo.commandBufferCount = _count;
	
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::COMMAND_POOL, "Allocating " + std::to_string(_count) + std::string(_level == VK_COMMAND_BUFFER_LEVEL_PRIMARY ? " primary" : " secondary") + " command buffer" + std::string(_count == 1 ? "" : "s") + " for " + PoolTypeToString(poolType) + " pool", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result{ vkAllocateCommandBuffers(device.GetDevice(), &allocInfo, buffers.data()) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::COMMAND_POOL, result == VK_SUCCESS ? "success" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::COMMAND_POOL," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}

	return buffers;
}



void VulkanCommandPool::FreeCommandBuffer(VkCommandBuffer& _buffer)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::COMMAND_POOL, "Freeing 1 command buffer\n");
	vkFreeCommandBuffers(device.GetDevice(), pool, 1, &_buffer);
}



void VulkanCommandPool::FreeCommandBuffers(std::vector<VkCommandBuffer>& _buffers)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::COMMAND_POOL, "Freeing " + std::to_string(_buffers.size()) + " command buffer" + std::string(_buffers.size() == 1 ? "" : "s") + "\n");
	vkFreeCommandBuffers(device.GetDevice(), pool, _buffers.size(), _buffers.data());
}



void VulkanCommandPool::Reset(VkCommandPoolResetFlags _flags)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::COMMAND_POOL, "Resetting command pool\n");
	vkResetCommandPool(device.GetDevice(), pool, _flags);
}




std::string VulkanCommandPool::PoolTypeToString(VK_COMMAND_POOL_TYPE _poolType)
{
	switch (_poolType)
	{
		case VK_COMMAND_POOL_TYPE::GRAPHICS:	return "GRAPHICS";
		case VK_COMMAND_POOL_TYPE::COMPUTE:		return "COMPUTE";
		default:								return "UNDEFINED";
	}
}



}