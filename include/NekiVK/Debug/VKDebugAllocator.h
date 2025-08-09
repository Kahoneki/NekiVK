#ifndef VKDEBUGALLOCATOR_H
#define VKDEBUGALLOCATOR_H

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <mutex>

namespace Neki
{


enum class VK_ALLOCATOR_TYPE
{
	DEFAULT, //The built-in vulkan allocator
	DEBUG, //VKDebugAllocator with verbose=false
	DEBUG_VERBOSE, //VKDebugAllocator with verbose=true
};


class VKDebugAllocator
{
public:
	VKDebugAllocator(const VK_ALLOCATOR_TYPE _type);
	~VKDebugAllocator();

	explicit inline operator const VkAllocationCallbacks*() const
	{
		//If type is set to default, return nullptr - vulkan interprets this as the built-in allocator
		return (type == VK_ALLOCATOR_TYPE::DEFAULT) ? nullptr : &callbacks;
	}

private:
	VkAllocationCallbacks callbacks;

	static void* VKAPI_CALL Allocation(void* _pUserData, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
	static void* VKAPI_CALL Reallocation(void* _pUserData, void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
	static void VKAPI_CALL Free(void* _pUserData, void* _pMemory);
	//todo: maybe add internal allocation notification callbacks ?

	void* AllocationImpl(std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
	void* ReallocationImpl(void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
	void FreeImpl(void* _pMemory);

	//Stores the sizes of all allocations (required for reallocation)
	std::unordered_map<void*, std::size_t> allocationSizes;
	//Mutex for accessing allocationSizes in a thread safe manner
	std::mutex allocationSizesMtx;
	
	const VK_ALLOCATOR_TYPE type;
};



}

#endif //VKDEBUGALLOCATOR_H
