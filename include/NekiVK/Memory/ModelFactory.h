#ifndef MODELFACTORY_H
#define MODELFACTORY_H



#include "BufferFactory.h"
#include "../Utils/Loaders/ImageLoader.h"
#include "../Core/VulkanCommandPool.h"
#include "../Utils/Loaders/ModelLoader.h"


//Helper class to load models into a vector of GPUMesh objects
namespace Neki
{



struct GPUMesh
{
	VkBuffer vertexBuffer;
	VkBuffer indexBuffer;
	std::uint32_t indexCount;
	std::vector<TextureInfo> textures;
};


class ModelFactory
{
public:
	explicit ModelFactory(const VKLogger& _logger,
						  VKDebugAllocator& _deviceDebugAllocator,
						  const VulkanDevice& _device,
						  BufferFactory& _bufferFactory);

	~ModelFactory();


	//Load data for a single model at _filepath into a vector of GPUMeshes comprising the model
	//Optionally pass in additional flags for the vertex and index buffers (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT and VK_BUFFER_USAGE_INDEX_BUFFER_BIT are added automatically)
	[[nodiscard]] std::vector<GPUMesh> LoadModel(const char* _filepath, const VkBufferUsageFlags _vertexBufferFlags=0, const VkBufferUsageFlags _indexBufferFlags=0);

	//Load model data for _count models at _filepaths into a vector of vectors of GPUMeshes comprising the corresponding model
	//E.g.: LoadModel(...)[1][2] is mesh index 2 of model index 1
	//Optionally pass in additional flags for the vertex and index buffers (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT and VK_BUFFER_USAGE_INDEX_BUFFER_BIT are added automatically)
	[[nodiscard]] std::vector<std::vector<GPUMesh>> LoadModels(std::uint32_t _count, const char** _filepaths, const VkBufferUsageFlags* _vertexBufferFlags=nullptr, const VkBufferUsageFlags* _indexBufferFlags=nullptr);


private:
	[[nodiscard]] std::vector<GPUMesh> LoadModelImpl(const char* _filepath, const VkBufferUsageFlags _vertexBufferFlags, const VkBufferUsageFlags _indexBufferFlags);

	//Dependency injections from VKApp
	const VKLogger& logger;
	VKDebugAllocator& deviceDebugAllocator;
	const VulkanDevice& device;
	BufferFactory& bufferFactory;
};



}



#endif