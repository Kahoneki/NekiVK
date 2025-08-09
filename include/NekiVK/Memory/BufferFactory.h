#ifndef BUFFERFACTORY_H
#define BUFFERFACTORY_H

#include "../Core/VulkanCommandPool.h"
#include "../Core/VulkanDevice.h"

//Responsible for the initialisation, ownership, and clean shutdown of VkBuffers and accompanying VkDeviceMemorys
namespace Neki
{


//For internal use only
struct BufferMetadata
{
	VkDeviceSize size;
	VkBufferUsageFlags usage;
	VkSharingMode sharingMode;
	VkMemoryPropertyFlags flags;
};


class BufferFactory
{
public:
	explicit BufferFactory(const VKLogger& _logger,
						   VKDebugAllocator& _deviceDebugAllocator,
						   const VulkanDevice& _device,
						   VulkanCommandPool& _commandPool);

	~BufferFactory();

	//Allocate a single buffer
	[[nodiscard]] VkBuffer AllocateBuffer(const VkDeviceSize& _size, const VkBufferUsageFlags& _usage, const VkSharingMode& _sharingMode=VK_SHARING_MODE_EXCLUSIVE, const VkMemoryPropertyFlags _requiredMemFlags=0);

	//Allocate multiple buffers from this pool
	[[nodiscard]] std::vector<VkBuffer> AllocateBuffers(std::uint32_t _count, const VkDeviceSize* _sizes, const VkBufferUsageFlags* _usages, const VkSharingMode* _sharingModes=nullptr, const VkMemoryPropertyFlags* _requiredMemFlags=nullptr);

	//Free a specific buffer
	void FreeBuffer(VkBuffer& _buffer);

	//Free a list of _count buffers
	void FreeBuffers(std::uint32_t _count, VkBuffer* _buffers);

	//Copies a host-visible buffer to a new device-local buffer and deletes the source buffer
	//Buffer must have been allocated with BufferFactory
	//The source buffer must have been created with the VK_BUFFER_USAGE_TRANSFER_SRC_BIT flag
	//Optionally, set freeSourceBuffer=true to free the source buffer (note this will invalidate any currently active memory maps on the source buffer)
	//Optionally, pass a (already begun) command buffer to this function and the barrier command will be recorded to it but not executed
	//Leaving _commandBuffer as nullptr will cause the function to allocate its own command buffer and automatically submit it
	VkBuffer TransferToDeviceLocalBuffer(VkBuffer& _buffer, bool _freeSourceBuffer=false, VkCommandBuffer* _commandBuffer=nullptr);


	[[nodiscard]] VkDeviceMemory GetMemory(VkBuffer _buffer);

	
private:
	[[nodiscard]] VkBuffer AllocateBufferImpl(const VkDeviceSize& _size, const VkBufferUsageFlags& _usage, const VkSharingMode& _sharingMode, const VkMemoryPropertyFlags _requiredMemFlags);
	void FreeBufferImpl(VkBuffer& _buffer);
	
	//Dependency injections from VKApp
	const VKLogger& logger;
	VKDebugAllocator& deviceDebugAllocator;
	const VulkanDevice& device;
	VulkanCommandPool& commandPool;

	std::unordered_map<VkBuffer, VkDeviceMemory> bufferMemoryMap;
	std::unordered_map<VkBuffer, BufferMetadata> bufferMetadataMap;
};



}

#endif