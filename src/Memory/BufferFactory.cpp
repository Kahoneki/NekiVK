#include "NekiVK/Memory/BufferFactory.h"
#include "NekiVK/Utils/Strings/format.h"

#include <stdexcept>
#include <algorithm>


namespace Neki
{



BufferFactory::BufferFactory(const VKLogger& _logger, VKDebugAllocator& _deviceDebugAllocator, const VulkanDevice& _device, VulkanCommandPool& _commandPool)
							: logger(_logger), deviceDebugAllocator(_deviceDebugAllocator), device(_device), commandPool(_commandPool)
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::BUFFER_FACTORY, "\n\n\n", VK_LOGGER_WIDTH::DEFAULT, false);
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::BUFFER_FACTORY, "Buffer Factory Initialised\n");
}



BufferFactory::~BufferFactory()
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::BUFFER_FACTORY,"Shutting down BufferFactory\n");
	while (!bufferMemoryMap.empty())
	{
		VkBuffer buffer{ bufferMemoryMap.begin()->first };
		FreeBufferImpl(buffer);
	}
	logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::BUFFER_FACTORY, "  All buffers and underlying memory freed\n");
}



VkBuffer BufferFactory::AllocateBuffer(const VkDeviceSize& _size, const VkBufferUsageFlags& _usage, const VkSharingMode& _sharingMode, const VkMemoryPropertyFlags _requiredMemFlags)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::BUFFER_FACTORY,"Allocating 1 Buffer And Associated Memory\n");
	return AllocateBufferImpl(_size, _usage, _sharingMode, _requiredMemFlags);
}



std::vector<VkBuffer> BufferFactory::AllocateBuffers(std::uint32_t _count, const VkDeviceSize* _sizes, const VkBufferUsageFlags* _usages, const VkSharingMode* _sharingModes, const VkMemoryPropertyFlags* _requiredMemFlags)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::BUFFER_FACTORY,"Allocating " + std::to_string(_count) + " Buffer" + std::string(_count == 1 ? "" : "s") + " And Associated Memory\n", VK_LOGGER_WIDTH::DEFAULT, false);
	std::vector<VkBuffer> buffers;
	for (std::size_t i{ 0 }; i<_count; ++i)
	{
		buffers.push_back(AllocateBufferImpl(_sizes[i], _usages[i], _sharingModes[i], _requiredMemFlags[i]));
	}
	return buffers;
}



void BufferFactory::FreeBuffer(VkBuffer& _buffer)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::BUFFER_FACTORY,"Freeing 1 Buffer And Associated Memory\n");
	FreeBufferImpl(_buffer);
}



void BufferFactory::FreeBuffers(std::uint32_t _count, VkBuffer* _buffers)
{
	logger.Log(VK_LOGGER_CHANNEL::HEADING, VK_LOGGER_LAYER::BUFFER_FACTORY,"Freeing " + std::to_string(_count) + " Buffer" + std::string(_count == 1 ? "" : "s") + " And Associated Memory\n");
	for (std::size_t i{ 0 }; i<_count; ++i)
	{
		FreeBufferImpl(_buffers[i]);
	}
}



VkBuffer BufferFactory::TransferToDeviceLocalBuffer(VkBuffer& _buffer, bool _freeSourceBuffer, VkCommandBuffer* _commandBuffer)
{
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::BUFFER_FACTORY,"Transferring Host-Visible Buffer To Device-Local Heap\n");
	if ((bufferMetadataMap[_buffer].usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) == 0)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::BUFFER_FACTORY, "  Source buffer wasn't created with the TRANSFER_SRC_BIT usage flag\n");
		throw std::runtime_error("");
	}
	if ((bufferMetadataMap[_buffer].flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::BUFFER_FACTORY, "  Source buffer wasn't created with the HOST_VISIBLE memory property flag\n");
		throw std::runtime_error("");
	}

	//For new buffer, remove TRANSFER_SRC_BIT usage flag and add TRANSFER_DST_BIT
	VkBufferUsageFlags newUsageFlags{ (bufferMetadataMap[_buffer].usage & (~VK_BUFFER_USAGE_TRANSFER_SRC_BIT)) | VK_BUFFER_USAGE_TRANSFER_DST_BIT };
	VkBuffer dstBuffer{ AllocateBufferImpl(bufferMetadataMap[_buffer].size, newUsageFlags, bufferMetadataMap[_buffer].sharingMode, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) };

	//Record command buffer for copy command
	VkCommandBuffer commandBuffer{ _commandBuffer == nullptr ? commandPool.AllocateCommandBuffer() : *_commandBuffer };
	if (_commandBuffer == nullptr)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
	}

	VkBufferCopy region{};
	region.size = bufferMetadataMap[_buffer].size;
	region.srcOffset = 0;
	region.dstOffset = 0;
	vkCmdCopyBuffer(commandBuffer, _buffer, dstBuffer, 1, &region);

	if (_commandBuffer == nullptr)
	{
		vkEndCommandBuffer(commandBuffer);

		//Execute the command buffer
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		vkQueueSubmit(device.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(device.GetGraphicsQueue());
		commandPool.FreeCommandBuffer(commandBuffer);
	}

	if (_freeSourceBuffer)
	{
		FreeBuffer(_buffer);
	}

	return dstBuffer;
}



VkDeviceMemory BufferFactory::GetMemory(VkBuffer _buffer)
{
	return bufferMemoryMap[_buffer];
}



VkBuffer BufferFactory::AllocateBufferImpl(const VkDeviceSize& _size, const VkBufferUsageFlags& _usage, const VkSharingMode& _sharingMode, const VkMemoryPropertyFlags _requiredMemFlags)
{
	VkBuffer buffer;
	
	//Create buffer
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.size = _size; //1 MiB
	bufferInfo.usage = _usage;
	bufferInfo.sharingMode = _sharingMode;
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::BUFFER_FACTORY, "  Creating buffer (size: " + GetFormattedSizeString(bufferInfo.size) + ")", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	VkResult result{ vkCreateBuffer(device.GetDevice(), &bufferInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &buffer) };
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::BUFFER_FACTORY, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::BUFFER_FACTORY," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}

	//Check memory requirements of buffer
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device.GetDevice(), buffer, &memRequirements);
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::BUFFER_FACTORY, "  Internal buffer memory requirements:\n");
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::BUFFER_FACTORY, "  - " + GetFormattedSizeString(memRequirements.size) + " minimum allocation size\n");
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::BUFFER_FACTORY, "  - " + std::to_string(memRequirements.alignment) + "-byte allocation alignment\n");
	std::string allowedMemTypeIndicesStr{};
	bool isFirst{ true };
	for (std::uint32_t i{ 0 }; i < 32; ++i)
	{
		if (memRequirements.memoryTypeBits & (1 << i))
		{
			if (!isFirst) { allowedMemTypeIndicesStr += ", "; }
			allowedMemTypeIndicesStr += std::to_string(i);
			isFirst = false;
		}
	}
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::BUFFER_FACTORY, "  - Allocated to memory-type-index in {" + allowedMemTypeIndicesStr + "}\n");

	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::BUFFER_FACTORY, "  User-requested buffer memory requirements:\n");
	std::string memTypePropFlagsString;
	if (_requiredMemFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) { memTypePropFlagsString += "DEVICE_LOCAL | "; }
	if (_requiredMemFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) { memTypePropFlagsString += "HOST_VISIBLE | "; }
	if (_requiredMemFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) { memTypePropFlagsString += "HOST_COHERENT | "; }
	if (_requiredMemFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) { memTypePropFlagsString += "HOST_CACHED | "; }
	if (_requiredMemFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) { memTypePropFlagsString += "LAZILY_ALLOCATED | "; }
	if (_requiredMemFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT) { memTypePropFlagsString += "PROTECTED | "; }
	if (_requiredMemFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) { memTypePropFlagsString += "DEVICE_COHERENT_AMD | "; }
	if (_requiredMemFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) { memTypePropFlagsString += "DEVICE_UNCACHED_AMD | "; }
	if (_requiredMemFlags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV) { memTypePropFlagsString += "RDMA_CAPABLE_NV | "; }
	if (!memTypePropFlagsString.empty())
	{
		memTypePropFlagsString.resize(memTypePropFlagsString.length() - 3);
	}
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::BUFFER_FACTORY, "  - Memory flag bits: " + std::string(memTypePropFlagsString.empty() ? "NONE" : memTypePropFlagsString) + "\n");

	//Find a memory type that fits the requirements
	std::uint32_t memTypeIndex{ UINT32_MAX };
	VkPhysicalDeviceMemoryProperties physicalDeviceMemProps;
	vkGetPhysicalDeviceMemoryProperties(device.GetPhysicalDevice(), &physicalDeviceMemProps);
	for (std::uint32_t i{ 0 }; i < physicalDeviceMemProps.memoryTypeCount; ++i)
	{
		//Check if this memory type is allowed for our buffer
		if (!(memRequirements.memoryTypeBits & (1 << i))) { continue; }
		if ((physicalDeviceMemProps.memoryTypes[i].propertyFlags & _requiredMemFlags) != _requiredMemFlags) { continue; }
		
		//This memory type is allowed
		memTypeIndex = i;
		logger.Log(VK_LOGGER_CHANNEL::SUCCESS, VK_LOGGER_LAYER::BUFFER_FACTORY, "  Found compatible memory type at index " + std::to_string(i) + "\n");
		break;
	}
	if (memTypeIndex == UINT32_MAX)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::BUFFER_FACTORY, "  No available memory types were found\n");
		throw std::runtime_error("");
	}

	//Allocate memory for the buffer
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memTypeIndex;
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::BUFFER_FACTORY, "  Allocating buffer memory", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	result = vkAllocateMemory(device.GetDevice(), &allocInfo, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator), &bufferMemoryMap[buffer]);
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::BUFFER_FACTORY, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::BUFFER_FACTORY," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}

	//Bind the allocated memory region to the buffer
	logger.Log(VK_LOGGER_CHANNEL::INFO, VK_LOGGER_LAYER::BUFFER_FACTORY, "  Binding buffer memory", VK_LOGGER_WIDTH::SUCCESS_FAILURE);
	result = vkBindBufferMemory(device.GetDevice(), buffer, bufferMemoryMap[buffer], 0);
	logger.Log(result == VK_SUCCESS ? VK_LOGGER_CHANNEL::SUCCESS : VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::BUFFER_FACTORY, result == VK_SUCCESS ? "success\n" : "failure", VK_LOGGER_WIDTH::DEFAULT, false);
	if (result != VK_SUCCESS)
	{
		logger.Log(VK_LOGGER_CHANNEL::ERROR, VK_LOGGER_LAYER::BUFFER_FACTORY," (" + std::to_string(result) + ")\n", VK_LOGGER_WIDTH::DEFAULT, false);
		throw std::runtime_error("");
	}

	//Populate buffer-metadata entry
	bufferMetadataMap[buffer].size = _size;
	bufferMetadataMap[buffer].usage = _usage;
	bufferMetadataMap[buffer].sharingMode = _sharingMode;
	bufferMetadataMap[buffer].flags = _requiredMemFlags;
	
	return buffer;
}



void BufferFactory::FreeBufferImpl(VkBuffer& _buffer)
{
	bufferMetadataMap.erase(_buffer);
	if (bufferMemoryMap[_buffer] != VK_NULL_HANDLE)
	{
		vkFreeMemory(device.GetDevice(), bufferMemoryMap[_buffer], static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
	}
	bufferMemoryMap.erase(_buffer);
	if (_buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device.GetDevice(), _buffer, static_cast<const VkAllocationCallbacks*>(deviceDebugAllocator));
	}
	_buffer = VK_NULL_HANDLE;
}



}