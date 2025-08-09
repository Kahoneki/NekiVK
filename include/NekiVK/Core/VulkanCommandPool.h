#ifndef VULKANCOMMANDPOOL_H
#define VULKANCOMMANDPOOL_H
#include "VulkanDevice.h"
#include <string>


//Responsible for:
//-The initialisation, ownership, and clean shutdown of a VkCommandPool object
//-The allocation of VkCommandBuffer objects from the VkCommandPool member
//
//The underlying command pool is not accessible, accessors must use provided functionality
namespace Neki
{

	//Used to specify the kind of command pool to create
	enum class VK_COMMAND_POOL_TYPE : std::uint32_t
	{
		GRAPHICS = 0,
		COMPUTE = 1,
		
		MAX_ENUM = 2,
	};

	class VulkanCommandPool
	{
	public:
		explicit VulkanCommandPool(const VKLogger& _logger,
								   VKDebugAllocator& _deviceDebugAllocator,
								   const VulkanDevice& _device,
								   VK_COMMAND_POOL_TYPE _poolType);

		~VulkanCommandPool();

		//Allocate a single command buffer from this pool
		[[nodiscard]] VkCommandBuffer AllocateCommandBuffer(VkCommandBufferLevel _level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		//Allocate multiple command buffers from this pool
		[[nodiscard]] std::vector<VkCommandBuffer> AllocateCommandBuffers(std::uint32_t _count, VkCommandBufferLevel _level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		//Free a specific command buffer
		void FreeCommandBuffer(VkCommandBuffer& _buffer);

		//Free a vector of command buffers
		void FreeCommandBuffers(std::vector<VkCommandBuffer>& _buffers);

		//Reset the entire pool, invalidating all allocated buffers
		void Reset(VkCommandPoolResetFlags _flags = 0);


	private:
		//Dependency injections from VKApp
		const VKLogger& logger;
		VKDebugAllocator& deviceDebugAllocator;
		const VulkanDevice& device;
		
		VkCommandPool pool;
		VK_COMMAND_POOL_TYPE poolType;

		static std::string PoolTypeToString(VK_COMMAND_POOL_TYPE _poolType);
	};
}



#endif