#include "NekiVK/Debug/VKDebugAllocator.h"
#include <cstdlib>
#include <iostream>
#include <cstring>
#if defined(_WIN32)
	#include <malloc.h>
#endif

namespace Neki
{



VKDebugAllocator::VKDebugAllocator(const VK_ALLOCATOR_TYPE _type) : type(_type), callbacks{}
{
	callbacks.pUserData = static_cast<void*>(this);
	callbacks.pfnAllocation = &Allocation;
	callbacks.pfnReallocation = &Reallocation;
	callbacks.pfnFree = &Free;

	if (type == VK_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VKDebugAllocator - Initialised.\n"; }
}


VKDebugAllocator::~VKDebugAllocator()
{
	//Check for memory leaks
	if (!allocationSizes.empty())
	{
		std::cerr << "ERR::VKDEBUGALLOCATOR::~VKDEBUGALLOCATOR::MEMORY_LEAK::ALLOCATOR_DESTRUCTED_WITH_" << std::to_string(allocationSizes.size()) << "_ALLOCATIONS_UNFREED" << std::endl;
	}

	if (type == VK_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VKDebugAllocator - Destroyed.\n"; }
}




void* VKAPI_CALL VKDebugAllocator::Allocation(void* _pUserData, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
{
	return static_cast<VKDebugAllocator*>(_pUserData)->AllocationImpl(_size, _alignment, _allocationScope);
}

void* VKAPI_CALL VKDebugAllocator::Reallocation(void* _pUserData, void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
{
	return static_cast<VKDebugAllocator*>(_pUserData)->ReallocationImpl(_pOriginal, _size, _alignment, _allocationScope);
}

void VKAPI_CALL VKDebugAllocator::Free(void* _pUserData, void* _pMemory)
{
	return static_cast<VKDebugAllocator*>(_pUserData)->FreeImpl(_pMemory);
}



void* VKDebugAllocator::AllocationImpl(std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
{
	if (type == VK_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VKDebugAllocator - AllocationImpl called for " << _size << " bytes aligned to " << _alignment << " bytes with " << _allocationScope << " allocation scope.\n"; }
	
	#if defined(_WIN32)
		void* ptr{ _aligned_malloc(_size, _alignment) };
		if (ptr == nullptr)
	#else
		void* ptr{ nullptr };
		if (posix_memalign(&ptr, _alignment, _size) != 0)
	#endif
	{
		std::cerr << "ERR::VKDEBUGALLOCATOR::ALLOCATIONIMPL::ALLOCATION_FAILED::RETURNING_NULLPTR" << std::endl;
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(allocationSizesMtx);
	allocationSizes[ptr] = _size;
	
	return ptr;
}



void* VKDebugAllocator::ReallocationImpl(void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
{
	if (type == VK_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VKDebugAllocator - ReallocationImpl called for address " << _pOriginal << " to the new pool of " << _size << " bytes aligned to " << _alignment << " bytes with " << _allocationScope << " allocation scope.\n"; }

	if (_pOriginal == nullptr)
	{
		if (type == VK_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VKDebugAllocator - ReallocationImpl - _pOriginal == nullptr, falling back to VKDebugAllocator::Allocation\n"; }
		return AllocationImpl(_size, _alignment, _allocationScope);
	}

	if (_size == 0)
	{
		if (type == VK_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VKDebugAllocator - ReallocationImpl - _size == 0, falling back to VKDebugAllocator::Free and returning nullptr\n"; }
		FreeImpl(_pOriginal);
		return nullptr;
	}
	
	//Get size of old allocation
	std::size_t oldSize;
	{
		std::lock_guard<std::mutex> lock(allocationSizesMtx);
		std::unordered_map<void*, std::size_t>::iterator it{ allocationSizes.find(_pOriginal) };
		if (it == allocationSizes.end())
		{
			std::cerr << "ERR::VKDEBUGALLOCATOR::REALLOCATIONIMPL::PROVIDED_DATA_NOT_ORIGINALLY_ALLOCATED_BY_VKDEBUGALLOCATOR::RETURNING_NULLPTR" << std::endl;
			return nullptr;
		}
		oldSize = it->second;
	}

	//Allocate a new block
	void* newPtr{ AllocationImpl(_size, _alignment, _allocationScope) };
	if (newPtr == nullptr)
	{
		std::cerr << "ERR::VKDEBUGALLOCATOR::REALLOCATIONIMPL::ALLOCATION_RETURNED_NULLPTR::RETURNING_NULLPTR" << std::endl;
		return nullptr;
	}

	//Copy data and free old block
	std::size_t bytesToCopy{ (oldSize < _size) ? oldSize : _size };
	memcpy(newPtr, _pOriginal, bytesToCopy);
	FreeImpl(_pOriginal);

	return newPtr;
}


void VKDebugAllocator::FreeImpl(void* _pMemory)
{
	if (type == VK_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VKDebugAllocator - FreeImpl called for address " << _pMemory << "\n"; }

	if (_pMemory == nullptr)
	{
		if (type == VK_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VKDebugAllocator - FreeImpl - _pMemory == nullptr, returning.\n"; }
		return;
	}
	
	//Remove from allocationSizes map
	std::lock_guard<std::mutex> lock(allocationSizesMtx);
	std::unordered_map<void*, std::size_t>::iterator it{ allocationSizes.find(_pMemory) };
	if (it == allocationSizes.end())
	{
		std::cerr << "ERR::VKDEBUGALLOCATOR::FREEIMPL::PROVIDED_DATA_NOT_ORIGINALLY_ALLOCATED_BY_VKDEBUGALLOCATOR::RETURNING" << std::endl;
		return;
	}
	allocationSizes.erase(it);

	#if defined(_WIN32)
		_aligned_free(_pMemory);
	#else
		free(_pMemory);
	#endif
	
}



}